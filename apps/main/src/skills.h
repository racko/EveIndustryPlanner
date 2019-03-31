#pragma once

#include <cstdint>
#include <unordered_set>
#include <unordered_map>

struct Names;

struct Skills {
    void loadSkillLevels(const Names& names);

    std::size_t getSkillLevel(std::size_t skill) const;

    bool isRacialEncryptionSkill(std::size_t skill) const;

    bool isAdvancedScienceSkill(std::size_t skill) const;

    bool isManufacturingTimeModifierScienceSkill(std::size_t skill) const;

    bool overrideSkills;

    std::size_t productionLines;
    std::size_t scienceLabs;
    double manufacturingTimeReduction;
    double copyTimeReduction;
    double researchTimeReduction;

    double cpu;
    double power;
private:
    std::unordered_set<std::size_t> racialEncryptionSkills;
    std::unordered_set<std::size_t> advancedScienceSkills;
    std::unordered_set<std::size_t> manufacturingTimeModifierSkills;
    std::unordered_map<std::size_t, std::size_t> skillLevels;
};
