#include "names.h"
#include <profiling.h>

#include <iostream>
#include <yaml-cpp/yaml.h>

const std::string& Names::getName(size_t typeId) const {
    return names.at(typeId); // could be changed to nothrow version when I check all names upon reading
}

const std::string& Names::getName(size_t typeId, const std::string& alternative) const {
    auto n = names.find(typeId);
    return n == names.end() ? alternative : n->second;
}

size_t Names::getTypeId(const std::string& name) const {
    return typeIds.at(name);
}

const std::string* Names::checkName(size_t typeId) const {
    auto n = names.find(typeId);
    return n == names.end() ? nullptr : &n->second;
}

void Names::loadNames(const YAML::Node& typesNode) {
    std::cout << "building map. " << std::flush;
    names.reserve(typesNode.size());
    auto tTime2 = time<float,std::milli>([&] {
        for (const auto& t : typesNode) {
            const auto& nameNode = t.second["name"]["en"];
            const auto& publishedNode = t.second["published"];
            if (!nameNode.IsDefined() || !publishedNode.IsDefined() || !publishedNode.as<bool>())
                continue;
            const auto& name = nameNode.as<std::string>();
            auto id = t.first.as<size_t>();
            names[id] = name;
            typeIds[name] = id;
        }
    });
    std::cout << tTime2.count() << "ms\n";
}

void Names::ensureName(size_t typeId, const std::string& name) {
    auto n = names.find(typeId);
    if (n == names.end()) {
        names[typeId] = name;
        typeIds[name] = typeId;
    }
}
