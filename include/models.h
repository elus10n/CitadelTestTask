#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <map>
#include <regex>
#include <functional>


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

struct ExtractionResult
{
    std::string filename;
    double value;
    std::string repr_value;

    ExtractionResult(const std::string& fn, const double val, const std::string& vv): filename(fn), value(val), repr_value(vv) {}
};

struct AggregationResult
{
    ExtractionResult max;
    ExtractionResult min;

    AggregationResult(const ExtractionResult& mx, const ExtractionResult& mn): max(mx), min(mn) {}
};

inline const std::map<std::string, RuleType> converter
{
    {"bool", RuleType::BOOL}, 
    {"value", RuleType::VALUE}, 
    {"speed", RuleType::SPEED}
};

using LogCallback = std::function<void(const std::string& error)>;

using WorkerOutput = std::map<std::string, std::map<std::string, std::vector<ExtractionResult>>>;//мап по имени сенсора в котором хранится мап по имени правила с записями :(
using AggregatorOutput = std::map<std::string, std::map<std::string, AggregationResult>>;//мап по имени сеносра в котором хранится мап по имени правила с аггрегированной статистикой

#endif