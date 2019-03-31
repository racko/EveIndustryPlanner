#include "settings.h"
#include <profiling.h>

#include <yaml-cpp/yaml.h>
#include <iostream>

void Settings::read() {
    YAML::Node settings;
    std::cout << "loading settings.yaml. " << std::flush;
    auto tTime = time<float,std::milli>([&]{ settings = YAML::LoadFile("settings.yaml"); });
    std::cout << tTime.count() << "ms.\n";

    skills.read(settings);
    trade.read(settings);
    facilities.read(settings);
    queries.read(settings);
    industry.read(settings);
    blueprints.read(settings);
}
