#include "libs/groups/groups.h"

#include "libs/profiling/profiling.h"
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/unordered_map.hpp>
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

bool Groups::operator==(const Groups& rhs) const { return groups == rhs.groups; }
bool Groups::operator!=(const Groups& rhs) const { return !(*this == rhs); }

void Groups::deserialize(std::istream& s) {
    boost::archive::binary_iarchive iarch(s);
    iarch >> groups;
}

void Groups::serialize(std::ostream& s) const {
    boost::archive::binary_oarchive oarch(s);
    oarch << groups;
}
