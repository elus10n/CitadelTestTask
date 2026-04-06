#ifndef AGGREGATOR_H
#define AGGREGATOR_H

#include "../../include/models.h"

class Aggregator
{
    LogCallback callback;

    bool has_warnings = false;

    std::map<std::string, AggregationResult> aggregateSensor(const std::map<std::string, std::vector<ExtractionResult>>& data);
    AggregationResult aggregateRule(const std::vector<ExtractionResult>& data);

    public:
    Aggregator() = default;

    AggregatorOutput aggregate(const WorkerOutput& data);

    bool hasWarnings() const {return has_warnings;}

    void setCallback(LogCallback cb) {callback = cb;}
};

#endif
