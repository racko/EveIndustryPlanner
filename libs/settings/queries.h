#pragma once

#include "libs/yaml_fwd/yaml_fwd.h"

#include <string>

struct Queries {
    std::string buyQuery;
    std::string sellQuery;
    void read(const YAML::Node& settings);
};
