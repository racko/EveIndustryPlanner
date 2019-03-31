#pragma once

#include <unordered_set>
#include "type_id.h"
#include "yaml_fwd.h"

struct BluePrintSelection {
    std::unordered_set<TypeID> ignoreList;
    double bpoPriceLimit;
    void read(const YAML::Node& settings);
};
