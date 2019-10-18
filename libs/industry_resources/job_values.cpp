#include "libs/industry_resources/job_values.h"

#include <iostream>

void JobValues::setJobValues(const std::unordered_map<std::size_t, double>& inputValues,
                             const std::unordered_map<std::size_t, std::size_t>& bpToProduct) {
    jobValues_.reserve(2 * bpToProduct.size());
    std::cout << "job values: \n";
    for (const auto& e : bpToProduct) {
        const auto bpId = e.first;
        const auto pId = e.second;
        const auto ivIt = inputValues.find(pId);
        if (ivIt == inputValues.end())
            continue;
        const auto inputValue = ivIt->second;
        jobValues_[pId] = inputValue;
        jobValues_[bpId] = 0.02 * inputValue;
        std::cout << "  " << pId << ": " << inputValue << '\n';
        std::cout << "  " << bpId << ": " << (0.02 * inputValue) << '\n';
    }
}

double JobValues::at(std::size_t id) const { return jobValues_.at(id); }

std::unordered_map<std::size_t, double>::const_iterator JobValues::find(const std::size_t id) const {
    return jobValues_.find(id);
}

std::unordered_map<std::size_t, double>::const_iterator JobValues::end() const { return jobValues_.end(); }
