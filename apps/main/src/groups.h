#pragma once

#include "yaml_fwd.h"
#include <cstddef>
#include <unordered_map>

struct Groups {
    void loadGroups(const YAML::Node& typesNode);
    const std::size_t* checkGroup(std::size_t typeId) const;

  private:
    std::unordered_map<std::size_t, std::size_t> groups;
};
