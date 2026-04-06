#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <regex>
#include <map>
#include <functional>
#include <fstream>
#include <unordered_set>

#include "nlohmann/json.hpp"
#include "models.h"


class ConfigParser
{
    std::vector<Sensor> sensors;
    std::map<std::string, Rule> rules;
    std::map<std::string, std::vector<std::string>> extractors;

    bool has_warnings = false;
    bool has_fatal = false;

    LogCallback callback;

    void setSensors(const nlohmann::json& sensors_);
    void setRules(const nlohmann::json& rules_);
    void setExtractors(const nlohmann::json& extractors_);

    public:

    ConfigParser() = default;

    void SetConfig(const std::string& path);

    std::vector<Sensor>& getSensors() {return sensors;}
    std::map<std::string, Rule>& getRules() {return rules;}
    std::map<std::string, std::vector<std::string>>& getExtractors() {return extractors;}

    void setCallback(LogCallback cb) {callback = cb;}

    bool hasWarnings() const {return has_warnings;}
    bool hasFatal() const {return has_fatal;}
};

#endif
