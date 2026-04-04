#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <regex>
#include <map>
#include <functional>
#include <fstream>
#include <unordered_set>

#include "../../include/nlohmann/json.hpp"
#include "../../include/models.h"


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

    const std::vector<Sensor>& getSensors() const {return sensors;}
    const std::map<std::string, Rule>& getRules() const {return rules;}
    const std::map<std::string, std::vector<std::string>>& getExtractors() const {return extractors;}

    void setCallback(LogCallback cb) {callback = cb;}

    bool hasWarnings() const {return has_warnings;}
    bool hasFatal() const {return has_fatal;}
};

#endif