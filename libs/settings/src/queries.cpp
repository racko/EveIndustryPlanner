#include "queries.h"

#include <yaml-cpp/yaml.h>

void Queries::read(const YAML::Node& settings) {
    buyQuery = settings["buyQuery"].as<std::string>();
    sellQuery = settings["sellQuery"].as<std::string>();
    // volumeQuery = settings["volumeQuery"].as<std::string>();
}
