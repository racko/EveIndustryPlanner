#pragma once

#include "libs/yaml_fwd/yaml_fwd.h"

struct FacilityCosts {
    double productionTax;
    double copyTax;
    double inventionTax;
    void read(const YAML::Node& settings);
};
