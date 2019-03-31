#pragma once

#include "yaml_fwd.h"

#include <cstddef>

struct IndustryLimits {
    double buyIn;
    double timeFrame;
    std::size_t productionLines;
    std::size_t scienceLabs;
    bool do_planetary_interaction;
    void read(const YAML::Node& settings);
};
