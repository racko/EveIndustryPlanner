#pragma once

#include <cstdint>
#include <unordered_map>

struct JobValues {
    std::unordered_map<std::size_t, double> jobValues;
    void setJobValues(const std::unordered_map<std::size_t,double>& inputValues, const std::unordered_map<std::size_t,std::size_t>& bpToProduct);
};
