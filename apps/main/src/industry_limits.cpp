#include "industry_limits.h"

#include <yaml-cpp/yaml.h>

void IndustryLimits::read(const YAML::Node& settings) {
    timeFrame = 3600 * settings["timeframe_h"].as<double>();
    productionLines = settings["lines"].as<size_t>();
    scienceLabs = settings["labs"].as<size_t>();
    do_planetary_interaction = settings["planetaryInteraction"].as<bool>();
    buyIn = settings["buy_in"].as<double>();
}
