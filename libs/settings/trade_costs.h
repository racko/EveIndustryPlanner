#pragma once

#include "yaml_fwd.h"

struct TradeCosts {
    double brokersFee;
    double salesTax;
    void read(const YAML::Node& settings);
};
