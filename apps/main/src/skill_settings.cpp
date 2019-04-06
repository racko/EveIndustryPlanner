#include "skill_settings.h"

#include <yaml-cpp/yaml.h>

void SkillSettings::read(const YAML::Node& settings) { overrideSkills = settings["override_skills"].as<bool>(); }
