#include "libs/settings/settings.h"
#include "libs/profiling/profiling.h"

#include <iostream>
#include <yaml-cpp/yaml.h>

void Settings::read() {
    YAML::Node settings;
    std::cout << "loading settings.yaml. " << std::flush;
    auto tTime = time<float, std::milli>([&] { settings = YAML::LoadFile("settings.yaml"); });
    std::cout << tTime.count() << "ms.\n";

    skills.read(settings);
    trade.read(settings);
    facilities.read(settings);
    queries.read(settings);
    industry.read(settings);
    blueprints.read(settings);
}
