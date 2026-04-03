#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <regex>
#include <map>
#include <functional>
#include <fstream>

struct Sensor
{
    std::string tech_name;
    std::string view_name;

    Sensor(const std::string& tn, const std::string& vn): tech_name(tn), view_name(vn) {}
};

enum class RuleType
{
    BOOL, VALUE, SPEED
};

struct Rule
{
    std::string name;
    RuleType type;
    std::regex pattern;
    std::string true_repr;
    std::string false_repr;

    Rule(const std::string& nm, const RuleType rt, const std::string& pt, const std::string& t = "", const std::string& f = ""): name(nm), type(rt), pattern(pt), true_repr(t), false_repr(f) {} 
};

using LogCallback = std::function<void(const std::string& error)>;

class ConfigParser
{
    std::vector<Sensor> sensors;
    std::map<std::string, Rule> rules;
    std::map<std::string, std::vector<std::string>> extractors;

    bool has_warnings = false;

    LogCallback callback;

    public:

    ConfigParser() = default;

    void SetConfig(const std::string& path);

    const std::vector<Sensor>& getSensors() const;
    const std::map<std::string, Rule>& getRules() const;
    const std::map<std::string, std::vector<std::string>>& getExtractors() const;

    void setCallback(LogCallback cb) {callback = cb;}

    bool hasWarnings() const {return has_warnings;}
};

#endif