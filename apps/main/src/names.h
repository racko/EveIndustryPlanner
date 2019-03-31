#pragma once

#include "yaml_fwd.h"
#include <cstddef>
#include <string>
#include <unordered_map>

struct Names {
    void loadNames(const YAML::Node& typesNode);
    const std::string& getName(size_t typeId) const;
    const std::string& getName(size_t typeId, const std::string& alternative) const;
    const std::string& getName(size_t typeId, std::string&& alternative) const = delete;
    size_t getTypeId(const std::string& name) const;
    const std::string* checkName(size_t typeId) const;
    void ensureName(size_t typeId, const std::string& name);

  private:
    std::unordered_map<size_t, std::string> names;
    std::unordered_map<std::string, size_t> typeIds;
};
