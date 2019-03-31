#pragma once

#include <cstdint>
#include <unordered_set>
#include <unordered_map>

struct Names;

struct Skills {
    void loadSkillLevels(const Names& names);

    size_t getSkillLevel(size_t skill) const;

    bool isRacialEncryptionSkill(size_t skill) const;

    bool isAdvancedScienceSkill(size_t skill) const;

    bool isManufacturingTimeModifierScienceSkill(size_t skill) const;

    bool overrideSkills;

    size_t productionLines;
    size_t scienceLabs;
    double manufacturingTimeReduction;
    double copyTimeReduction;
    double researchTimeReduction;

    double cpu;
    double power;
private:
    std::unordered_set<size_t> racialEncryptionSkills;
    std::unordered_set<size_t> advancedScienceSkills;
    std::unordered_set<size_t> manufacturingTimeModifierSkills;
    std::unordered_map<size_t, size_t> skillLevels;
};
