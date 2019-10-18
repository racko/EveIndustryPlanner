#pragma once

#include "libs/yaml_fwd/yaml_fwd.h"

#include <cstddef>

struct IndustryLimits {
    void read(const YAML::Node& settings);

    double buyIn;
    double timeFrame;
    std::size_t productionLines;
    std::size_t scienceLabs;
    bool do_planetary_interaction;
};
