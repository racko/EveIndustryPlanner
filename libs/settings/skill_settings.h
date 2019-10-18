#pragma once

#include "yaml_fwd.h"

struct SkillSettings {
    bool overrideSkills;
    void read(const YAML::Node& settings);
};
