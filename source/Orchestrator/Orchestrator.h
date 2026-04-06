#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <fstream>

#include "models.h"
#include "Parser.h"
#include "Worker.h"
#include "Aggregator.h"
#include "Logger.h"

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
