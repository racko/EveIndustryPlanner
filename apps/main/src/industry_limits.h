#pragma once

#include "yaml_fwd.h"

#include <cstddef>

struct IndustryLimits {
    double buyIn;
    double timeFrame;
    size_t productionLines;
    size_t scienceLabs;
    bool do_planetary_interaction;
    void read(const YAML::Node& settings);
};
