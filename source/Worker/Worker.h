#ifndef WORKER_H
#define WORKER_H

#include <vector>
#include <string>
#include <map>
#include <atomic>
#include <mutex>
#include <future>
#include <fstream>
#include <sstream>
#include <locale>
#include <cmath>
#include <limits>

#include "models.h"

class Worker
{
    std::vector<Sensor> sensors;
    std::map<std::string, Rule> rules;
    std::map<std::string, std::vector<std::string>> extractors;

    LogCallback callback;

    std::atomic<bool> has_warnings = false;
    std::atomic<bool> has_fatal = false;

    std::mutex callback_mutex;

    WorkerOutput processSingle(const std::string& file_path);

    double parseNumericValue(const std::string& repr_val, const Rule& rule);
    double convertToBits(double value, const std::string& unit);

    public: 

    Worker(std::vector<Sensor>& sens, std::map<std::string, Rule>& rul, std::map<std::string, std::vector<std::string>>& extr) : sensors(std::move(sens)), rules(std::move(rul)), extractors(std::move(extr)) {}

    WorkerOutput processFiles(const std::vector<std::string>& filePaths);

    void setCallback(LogCallback cb) {callback = cb;}

    bool hasWarnings() const {return has_warnings;}
    bool hasFatal() const {return has_fatal;}
};

#endif
