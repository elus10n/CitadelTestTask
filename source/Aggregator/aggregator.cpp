#include "aggregator.h"

AggregatorOutput Aggregator::aggregate(const WorkerOutput& data)
{
    AggregatorOutput result;

    for(const auto& [sensor, sensor_data] : data)
        result.emplace(sensor, aggregateSensor(sensor_data));

    return result;
}

std::map<std::string, AggregationResult> Aggregator::aggregateSensor(const std::map<std::string, std::vector<ExtractionResult>>& data)
{
    std::map<std::string, AggregationResult> result;

    for(const auto& [rule, rule_data] : data)
        result.emplace(rule, aggregateRule(rule_data));

    return result;
}

AggregationResult Aggregator::aggregateRule(const std::vector<ExtractionResult>& data)
{
    if(data.empty())
    {
        if(callback) callback("Empty vector in aggregator for one of the rules!");
        has_warnings = true;
        return {{"NO_DATA", 0.0, "NO_DATA"},{"NO_DATA", 0.0, "NO_DATA"}};
    }

    auto comparator = [](const ExtractionResult& left, const ExtractionResult& right) {return left.value < right.value;};

    auto [min_it, max_it] = std::minmax_element(data.begin(),data.end(), comparator);

    return {*max_it, *min_it};
}