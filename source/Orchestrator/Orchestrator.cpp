#include "orchestrator.h"

//так написано, что парсер и воркер сами проверяют необходимые им пути, нам надо проверить только путь к отчетам
void Orchestrator::launchParser(const std::string& config_path, const std::vector<std::string>& file_paths, const std::string& report_path)
{
    if(config_path.empty() || file_paths.empty() || report_path.empty())
    {
        if(callback) callback("Some of the input data (paths) are empty. Return");
        return;
    }

    std::ofstream report_stream(report_path);
    if(!report_stream.is_open())
    {
        if(callback) callback("Unable to open path to write report. Return");
        return;
    }

    //запуск парсера
    ConfigParser parser;
    parser.setCallback(callback);
    parser.SetConfig(config_path);
    if(parser.hasFatal())
    {
        if(callback) callback("Fatal in the configuration parser, check the logs. Return");
        return;
    }
    else if(parser.hasWarnings())
        if(callback) callback("Warnings in the configuration parser, check the logs. Keep in mind that the data may be incorrect.");

    //запуск воркера
    Worker worker(parser.getSensors(), parser.getRules(), parser.getExtractors());
    worker.setCallback(callback);
    WorkerOutput w_output = worker.processFiles(file_paths);
    if(worker.hasFatal())
    {
        if(callback) callback("Fatal in the worker, check the logs. Return");
        return;
    }
    else if(worker.hasWarnings())
        if(callback) callback("Warnings in the worker, check the logs. Keep in mind that the data may be incorrect.");

    //запуск аггрегатора
    Aggregator aggregator;
    aggregator.setCallback(callback);
    AggregatorOutput ag_output = aggregator.aggregate(w_output);
    if(aggregator.hasWarnings())
        if(callback) callback("Warnings in the aggregator, check the logs. Keep in mind that the data may be incorrect.");

    printResult(ag_output, report_stream);
}

void Orchestrator::printResult(const AggregatorOutput& data, std::ofstream& stream)
{
    for(const auto& [sensor, sensor_data] : data)
    {
        printSensor(sensor_data, stream);
        stream << std::endl;
    }
}

void Orchestrator::printSensor(const std::map<std::string, AggregationResult>& sensor_data, std::ofstream& stream)
{
    for(const auto& [rule, rule_data] : sensor_data)
        stream << rule << ": max=" << rule_data.max.repr_value << "(" << rule_data.max.filename << "), min="<<rule_data.min.repr_value<<"("<<rule_data.min.filename<< ")" << std::endl;
}