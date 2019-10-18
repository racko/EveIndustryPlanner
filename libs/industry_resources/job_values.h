#pragma once

#include <cstdint>
#include <unordered_map>

struct JobValues {
    void setJobValues(const std::unordered_map<std::size_t, double>& inputValues,
                      const std::unordered_map<std::size_t, std::size_t>& bpToProduct);

    double at(std::size_t id) const;
    std::unordered_map<std::size_t, double>::const_iterator find(std::size_t id) const;
    std::unordered_map<std::size_t, double>::const_iterator end() const;
  private:
    std::unordered_map<std::size_t, double> jobValues_;
};
