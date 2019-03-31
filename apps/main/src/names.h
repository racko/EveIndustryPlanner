#pragma once

#include "yaml_fwd.h"
#include <cstddef>
#include <string>
#include <unordered_map>

struct Names {
    void loadNames(const YAML::Node& typesNode);
    const std::string& getName(std::size_t typeId) const;
    const std::string& getName(std::size_t typeId, const std::string& alternative) const;
    const std::string& getName(std::size_t typeId, std::string&& alternative) const = delete;
    std::size_t getTypeId(const std::string& name) const;
    const std::string* checkName(std::size_t typeId) const;
    void ensureName(std::size_t typeId, const std::string& name);

  private:
    std::unordered_map<std::size_t, std::string> names;
    std::unordered_map<std::string, std::size_t> typeIds;
};
