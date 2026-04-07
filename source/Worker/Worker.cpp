#include "Worker.h"

std::string trim(const std::string& d_str) 
{
    size_t first = d_str.find_first_not_of(" \r\n\t");
    if (std::string::npos == first) return "";
    size_t last = d_str.find_last_not_of(" \r\n\t");
    return d_str.substr(first, (last - first + 1));
}

WorkerOutput Worker::processFiles(const std::vector<std::string>& filePaths)
{
    WorkerOutput final_output;
    std::vector<std::future<WorkerOutput>> futures;

    for(const auto& path : filePaths)
    {
        if(path.empty())
        {
            if(callback) callback("Error in worker: empty path to the file!");
            has_warnings = true;
            continue;
        }
        futures.push_back(std::async(std::launch::async, &Worker::processSingle, this, path));
    }

    for(auto& future : futures)
    {
        WorkerOutput single_output = future.get();
        for(auto& [sens_name, extr_map] : single_output)
            for(auto& [rule_name, extr_res] : extr_map)
            {
                auto& vec = final_output[sens_name][rule_name];
                vec.reserve(vec.size() + extr_res.size());
                vec.insert(vec.end(), std::make_move_iterator(extr_res.begin()), std::make_move_iterator(extr_res.end()));
            }
    }

    return final_output;
}

WorkerOutput Worker::processSingle(const std::string& file_path)
{
    WorkerOutput output;
    const Sensor* context = nullptr;

    std::ifstream stream(file_path);
    if(!stream.is_open())
    {
        std::lock_guard<std::mutex> lock(callback_mutex);
        if(callback) callback("Error opening file at path: " + file_path);
        has_warnings = true;
    }

    std::string line;
    while(std::getline(stream, line))
    {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if(line.empty() || line.find("//") == 0) continue;//пропускаем пустые строки и строки с комменатриями, чтобы не нагружать логгер

        bool sensor_found = false;
        for (const auto& s : sensors) 
        {
            if (line.find(s.view_name + ":") != std::string::npos) 
            {
                context = &s;
                sensor_found = true;
                break;
            }
        }
        if (sensor_found) continue;

        bool matched_any = false;
        if (context) 
        {
            const auto& sensor_rule_names = extractors.at(context->tech_name);

            for (const auto& r_name : sensor_rule_names) 
            {
                const auto& rule = rules.at(r_name);
                std::smatch match;

                if (std::regex_search(line, match, rule.pattern)) 
                {
                    std::string clean_match1 = trim(match[1].str());
                    std::string clean_match2 = (match.size() > 2) ? trim(match[2].str()) : "";

                    if(rule.type == RuleType::BOOL && clean_match1 != rule.false_repr && clean_match1 != rule.true_repr)
                    {
                        std::lock_guard<std::mutex> lock(callback_mutex);
                        if(callback) callback("Error in Worker class thread. Unknown state representation: " + clean_match1 + ". Possible options: " + rule.true_repr + " and " + rule.false_repr + ".");
                        has_warnings = true;
                        continue;
                    }
                    else if(rule.type == RuleType::SPEED && clean_match2 != "bit" && clean_match2 != "Kbit" && clean_match2 != "Mbit" && clean_match2 != "Gbit")
                    {
                        std::lock_guard<std::mutex> lock(callback_mutex);
                        if(callback) callback("Error in Worker class thread. Unknown speed representation: " + clean_match2 + ".");
                        has_warnings = true;
                        continue;
                    }

                    double val = parseNumericValue(clean_match1, rule);

                    if (rule.type == RuleType::SPEED && match.size() > 2) 
                        val = convertToBits(val, clean_match2);

                    std::string repr_value = (rule.type == RuleType::SPEED && match.size() > 2) ? (clean_match1 + " " + clean_match2 + "/s") : clean_match1;
                    output[context->tech_name][r_name].emplace_back(file_path, val, repr_value);
                    matched_any = true;
                    break; 
                }
            }
        }
        if(!sensor_found && !matched_any)
        {
            std::lock_guard<std::mutex> lock(callback_mutex);
            if(callback) callback("Unrecognized string: " + line);
            has_warnings = true;
        }
    }

    return output;
}

double Worker::parseNumericValue(const std::string& repr_val, const Rule& rule)
{
    RuleType type = rule.type;

    switch(type)
    {
        case RuleType::BOOL :
        {
            if(repr_val == rule.false_repr) return 0.0;
            else return 1.0;

            break;
        }
        case RuleType::SPEED: case RuleType::VALUE:
        {
            try
            {
                std::stringstream ss;
                ss.imbue(std::locale::classic()); 
                ss << trim(repr_val);
                double value = 0.0;
                ss >> value;
                return value;
            }
            catch(const std::exception& e)
            {
                std::lock_guard<std::mutex> lock(callback_mutex);
                if(callback) callback(std::string("Error in Worker class thread") + e.what() + ". The sensor is read as 0.0");
                has_warnings = true;
                return 0;
            }
            break;
        }
        default : //такая ситуация невозможна, т.к. на этапе парсинга JSON мы отбрасываем это правило
        {
            std::lock_guard<std::mutex> lock(callback_mutex);
            if(callback) callback("Error in Worker class thread. Unknown rule type.");
            has_warnings = true;
            return 0.0;
        }
    }
}

double Worker::convertToBits(double value, const std::string& unit)
{
    if(unit == "Gbit") return value * 1e9;
    else if(unit == "Mbit") return value * 1e6;
    else if(unit == "Kbit") return value * 1e3;
    else return value;
}

