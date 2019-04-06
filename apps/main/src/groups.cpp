#include "groups.h"
#include <profiling.h>

#include <iostream>
#include <yaml-cpp/yaml.h>

void Groups::loadGroups(const YAML::Node& typesNode) {
    std::cout << "building map. " << std::flush;
    groups.reserve(typesNode.size());
    const auto tTime2 = time<float, std::milli>([&] {
        for (const auto& t : typesNode) {
            const auto& groupIdNode = t.second["groupID"];
            const auto& publishedNode = t.second["published"];
            if (!groupIdNode.IsDefined() || !publishedNode.IsDefined() || !publishedNode.as<bool>())
                continue;
            const auto groupId = groupIdNode.as<std::size_t>();
            const auto id = t.first.as<std::size_t>();
            groups[id] = groupId;
        }
    });
    std::cout << tTime2.count() << "ms\n";
}

const std::size_t* Groups::checkGroup(std::size_t typeId) const {
    const auto g = groups.find(typeId);
    return g == groups.end() ? nullptr : &g->second;
}
