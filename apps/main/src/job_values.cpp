#include "job_values.h"

#include <iostream>

void JobValues::setJobValues(const std::unordered_map<std::size_t,double>& inputValues, const std::unordered_map<std::size_t,std::size_t>& bpToProduct) {
    jobValues.reserve(2 * bpToProduct.size());
    std::cout << "job values: \n";
    for (const auto& e : bpToProduct) {
        auto bpId = e.first;
        auto pId = e.second;
        auto ivIt = inputValues.find(pId);
        if (ivIt == inputValues.end())
            continue;
        auto inputValue = ivIt->second;
        jobValues[pId] = inputValue;
        jobValues[bpId] = 0.02 * inputValue;
        std::cout << "  " << pId << ": " << inputValue << '\n';
        std::cout << "  " << bpId << ": " << (0.02 * inputValue) << '\n';
    }
}
