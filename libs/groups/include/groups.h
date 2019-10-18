#pragma once

#include "yaml_fwd.h"
#include <cstddef>
#include <unordered_map>
#include <iosfwd>

struct Groups {
    void loadGroups(const YAML::Node& typesNode);
    const std::size_t* checkGroup(std::size_t typeId) const;

    bool operator==(const Groups&) const;
    bool operator!=(const Groups&) const;

    void serialize(std::ostream&) const;
    void deserialize(std::istream& s);

  private:
    std::unordered_map<std::size_t, std::size_t> groups;
};
