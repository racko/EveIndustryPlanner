#include "trade_costs.h"

#include <yaml-cpp/yaml.h>

void TradeCosts::read(const YAML::Node& settings) {
    brokersFee = settings["brokersFee"].as<double>();
    salesTax = settings["salesTax"].as<double>();
}
