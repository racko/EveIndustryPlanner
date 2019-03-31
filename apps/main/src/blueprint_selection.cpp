#include "blueprint_selection.h"

#include <yaml-cpp/yaml.h>

void BluePrintSelection::read(const YAML::Node& settings) {
    bpoPriceLimit = settings["max_bpo_price"].as<double>();
    for (const auto& i : settings["ignore"])
        ignoreList.insert(i.as<size_t>());
}
