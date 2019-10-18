#pragma once

#include "libs/yaml_fwd/yaml_fwd.h"
#include <cstddef>
#include <iosfwd>
#include <string>
#include <unordered_map>

struct Names {
    void loadNames(const YAML::Node& typesNode);
    const std::string& getName(std::size_t typeId) const;
    const std::string& getName(std::size_t typeId, const std::string& alternative) const;
    std::string getName(std::size_t typeId, std::string&& alternative) const;
    std::size_t getTypeId(const std::string& name) const;
    const std::string* checkName(std::size_t typeId) const;
    void ensureName(std::size_t typeId, const std::string& name);

    void serialize(std::ostream&) const;
    void deserialize(std::istream& s);

    bool operator==(const Names&) const;
    bool operator!=(const Names&) const;

  private:
    std::unordered_map<std::size_t, std::string> names;
    std::unordered_map<std::string, std::size_t> typeIds;
};
