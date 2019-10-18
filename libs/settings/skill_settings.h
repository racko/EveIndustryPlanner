#pragma once

#include "libs/yaml_fwd/yaml_fwd.h"

struct SkillSettings {
    bool overrideSkills;
    void read(const YAML::Node& settings);
};
