#include "parser.h"

void ConfigParser::SetConfig(const std::string& path)
{
    if(path.empty())
    {
        if(callback) callback("Empty path to the JSON configuration!");
        has_warnings = true;
        has_fatal = true;
        return;
    }

    std::ifstream file(path);
    if(!file.is_open())
    {
        if(callback) callback("Error when opening a file: " + path);
        has_warnings = true;
        has_fatal = true;
        return;
    }

    nlohmann::json json;
    try
    {
        json = nlohmann::json::parse(file);
    }
    catch(const nlohmann::json::parse_error& e)
    {
        if(callback) callback("Error when parsing the configuration file: " + path + " | " + e.what());        
        has_warnings = true;
        has_fatal = true;
        return;
    }

    nlohmann::json sensors_;
    nlohmann::json rules_;
    nlohmann::json extractors_;
    try
    {
        sensors_ = json.at("sensors");
        rules_ = json.at("rules");
        extractors_ = json.at("extractors");
    }
    catch(const nlohmann::json::exception& e)
    {
        if(callback) callback("Error when parsing the configuration file: " + path + " | " + e.what());        
        has_warnings = true;
        has_fatal = true;
        return;
    }

    setSensors(sensors_);
    setRules(rules_);
    setExtractors(extractors_);

    sensors.erase(
        std::remove_if(sensors.begin(), sensors.end(), 
            [this](const Sensor& s) 
            {
                bool has_no_rules = (extractors.find(s.tech_name) == extractors.end()) || extractors.at(s.tech_name).empty();
                if (has_no_rules) 
                {
                    if(callback) callback("Sensor removed due to lack of valid rules: " + s.tech_name);
                    has_warnings = true;
                }
                return has_no_rules;
            }), 
        sensors.end()
    );

    if (sensors.empty()) 
    {
        if (callback) callback("Fatal: No valid sensors with rules remain after filtering.");
        has_fatal = true;
        return;
    }
}

void ConfigParser::setSensors(const nlohmann::json& sensors_)
{
    for(const auto& sensor : sensors_)
    {
        try
        {
            std::string tech_name = sensor.at("name");
            std::string view_name = sensor.at("rule");
            sensors.emplace_back(tech_name, view_name);
        }
        catch(const nlohmann::json::exception& e)
        {
            if(callback) callback(std::string("Error parsing the 'sensors' section in JSON: ") + e.what());        
            has_warnings = true;
        }
    }
}

void ConfigParser::setRules(const nlohmann::json& rules_)
{
    for(const auto& rule : rules_)
    {
        try
        {
            std::string name = rule.at("name");
            RuleType rule_t = converter.at(rule.at("type"));
            std::string reg = rule.at("rule");

            if(rule_t == RuleType::BOOL)
            {
                std::string true_repr;
                std::string false_repr;
                try
                {
                    true_repr = rule.at("true");
                    false_repr = rule.at("false");
                }
                catch(const nlohmann::json::exception& e)
                {
                    if(callback) callback(std::string("Error parsing the 'rules' section in JSON: ") + e.what()); 
                    has_warnings = true;
                    continue;
                }

                rules.emplace(name, Rule{name, rule_t, reg, true_repr, false_repr});
            }
            else rules.emplace(name, Rule{name, rule_t, reg});
        }
        catch(const nlohmann::json::exception& e)
        {
            if(callback) callback(std::string("Error parsing the 'rules' section in JSON: ") + e.what());        
            has_warnings = true;
        }
        catch(const std::regex_error& e)
        {
            if(callback) callback(std::string("Error parsing the 'rules' section in JSON: ") + e.what());        
            has_warnings = true;
        }
        catch(const std::exception& e)
        {
            if(callback) callback(std::string("Error parsing the 'rules' section in JSON: ") + e.what());        
            has_warnings = true;
        }
    }
}

void ConfigParser::setExtractors(const nlohmann::json& extractors_)
{
    std::unordered_set<std::string> known_sensors;
    for (const auto& s : sensors)
        known_sensors.insert(s.tech_name);

    std::unordered_set<std::string> known_rules;
    for (const auto& r : rules)
        known_rules.insert(r.first);

    for(const auto& extract : extractors_)
    {
        try
        {
            std::string sensor = extract.at("sensor");
            if(known_sensors.find(sensor) == known_sensors.end())
            {
                has_warnings = true;
                if(callback) callback("Unable to define rules for a non-existent sensor: " + sensor);
            }

            std::vector<std::string> tmp_rules;
            auto list = extract.at("rules");
            for(const auto& rule : list)
            {

                if(known_rules.find(rule) == known_rules.end())
                {
                    has_warnings = true;
                    if(callback) callback(std::string("Warning parsing the 'extractors' section in JSON: unknown rule for " + sensor));      
                    continue; 
                }
                tmp_rules.push_back(rule);
            }

            if(tmp_rules.empty())
            {
                has_warnings = true;
                if(callback) callback(std::string("Warning parsing the 'extractors' section in JSON: empty rules list for " + sensor));      
                continue;  
            }

            auto& existing_rules = extractors[sensor];
            for(const auto& r : tmp_rules) 
                if (std::find(existing_rules.begin(), existing_rules.end(), r) == existing_rules.end())
                    existing_rules.push_back(r);
        }
        catch(const nlohmann::json::exception& e)
        {
            if(callback) callback(std::string("Error parsing the 'extractors' section in JSON: ") + e.what());        
            has_warnings = true;
        }
    }
}
