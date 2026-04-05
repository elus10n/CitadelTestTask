#ifndef AGGREGATOR_H
#define AGGREGATOR_H

#include "../../include/models.h"

class Aggregator
{
    LogCallback callback;

    std::map<std::string, AggregationResult> aggregateSensor(const std::map<std::string, std::vector<ExtractionResult>>& data) const;
    AggregationResult aggregateRule(const std::vector<ExtractionResult>& data) const;

    public:
    Aggregator() = default;

    AggregatorOutput aggregate(const WorkerOutput& data) const;

    void setCallback(LogCallback cb) {callback = cb;}
};

#endif