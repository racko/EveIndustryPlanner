#include "facility_costs.h"

#include <yaml-cpp/yaml.h>

void FacilityCosts::read(const YAML::Node& settings) {
    productionTax = 1.1 * settings["facility_tax"]["production"].as<double>();
    copyTax = 1.1 * settings["facility_tax"]["copy"].as<double>();
    inventionTax = 1.1 * settings["facility_tax"]["invention"].as<double>();
}
