#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <fstream>

#include "../../include/models.h"
#include "../parser/parser.h"
#include "../worker/worker.h"
#include "../aggregator/aggregator.h"
#include "../logger/logger.h"

class Orchestrator
{
    LogCallback callback;

    void printSensor(const std::map<std::string, AggregationResult>& sensor_data, std::ofstream& stream);
    void printResult(const AggregatorOutput& data, std::ofstream& stream);

    public:
    Orchestrator() = default;

    void launchParser(const std::string& config_path, const std::vector<std::string>& file_paths, const std::string& report_path);

    void setCallback(LogCallback cb) {callback = cb;}
};

#endif