#pragma once

#include "libs/industry_resources/type_id.h"
#include "libs/yaml_fwd/yaml_fwd.h"
#include <unordered_set>

struct BluePrintSelection {
    std::unordered_set<TypeID> ignoreList;
    double bpoPriceLimit;
    void read(const YAML::Node& settings);
};
