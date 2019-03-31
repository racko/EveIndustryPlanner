#pragma once

#include <cstdint>
#include <unordered_map>

struct JobValues {
    std::unordered_map<size_t, double> jobValues;
    void setJobValues(const std::unordered_map<size_t,double>& inputValues, const std::unordered_map<size_t,size_t>& bpToProduct);
};
