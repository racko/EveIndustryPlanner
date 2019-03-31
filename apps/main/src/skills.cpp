#include "skills.h"
#include "names.h"

#include <iostream>
#include <pugixml.hpp>

bool Skills::isRacialEncryptionSkill(std::size_t skill) const {
    return racialEncryptionSkills.find(skill) != racialEncryptionSkills.end();
}

bool Skills::isAdvancedScienceSkill(std::size_t skill) const {
    return advancedScienceSkills.find(skill) != advancedScienceSkills.end();
}

bool Skills::isManufacturingTimeModifierScienceSkill(std::size_t skill) const {
    return manufacturingTimeModifierSkills.find(skill) != manufacturingTimeModifierSkills.end();
}

void Skills::loadSkillLevels(const Names& names) {
    skillLevels.reserve(100);
    std::cout << "Skills:\n";
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file("charactersheet.xml");
    if (!result) {
        std::cout << "Error description: " << result.description() << "\n";
        std::cout << "Error offset: " << result.offset << "\n\n";
        throw std::runtime_error("parsing character sheet failed.");
    }
    static const auto skillsString = std::string("skills");
    const auto& skills = doc.child("eveapi").child("result").find_child([] (const pugi::xml_node& n) { return n.attribute("name").as_string() == skillsString; }).children();
    for (const auto& skill : skills) {
        auto typeId = skill.attribute("typeID").as_ullong();
        auto level = skill.attribute("level").as_ullong();
        std::cout << "  " << names.getName(typeId) << " (" << typeId << "): " << level << '\n';
        skillLevels.insert({ typeId, level });
    }

    const auto industry = getSkillLevel(names.getTypeId("Industry"));
    const auto science = getSkillLevel(names.getTypeId("Science"));
    const auto advancedIndustry = getSkillLevel(names.getTypeId("Advanced Industry"));
    const auto commandCenterUpgrades = getSkillLevel(names.getTypeId("Command Center Upgrades"));
    const auto interplanetaryConsolidation = getSkillLevel(names.getTypeId("Interplanetary Consolidation"));

    productionLines = 1 + getSkillLevel(names.getTypeId("Mass Production")) + getSkillLevel(names.getTypeId("Advanced Mass Production"));
    scienceLabs = 1 + getSkillLevel(names.getTypeId("Laboratory Operation")) + getSkillLevel(names.getTypeId("Advanced Laboratory Operation"));
    manufacturingTimeReduction = (1.0 - double(industry) * 0.04) * (1.0 - double(advancedIndustry) * 0.03);
    copyTimeReduction = (1.0 - double(science) * 0.05) * (1.0 - double(advancedIndustry) * 0.03);
    researchTimeReduction = (1.0 - double(advancedIndustry) * 0.03);

    std::array<double, 6> powerLevels{{6000.0, 9000.0, 12000.0, 15000.0, 17000.0, 19000.0}};
    std::array<double, 6> cpuLevels{{1675.0, 7057.0, 12136.0, 17215.0, 21315.0, 25415.0}};

    cpu = double((1 + interplanetaryConsolidation)) * (cpuLevels[commandCenterUpgrades] - 2.0 * (3600.0 + 22.0));
    power = double((1 + interplanetaryConsolidation)) * (powerLevels[commandCenterUpgrades] - 2.0 * (700 + 15.0));

    racialEncryptionSkills = std::unordered_set<std::size_t> {
        names.getTypeId("Amarr Encryption Methods"),
        names.getTypeId("Caldari Encryption Methods"),
        names.getTypeId("Gallente Encryption Methods"),
        names.getTypeId("Minmatar Encryption Methods"),
        names.getTypeId("Sleeper Encryption Methods")
    };
    advancedScienceSkills = std::unordered_set<std::size_t> {
        names.getTypeId("Amarr Starship Engineering"),
        names.getTypeId("Caldari Starship Engineering"),
        names.getTypeId("Gallente Starship Engineering"),
        names.getTypeId("Minmatar Starship Engineering"),
        names.getTypeId("Electromagnetic Physics"),
        names.getTypeId("Electronic Engineering"),
        names.getTypeId("Graviton Physics"),
        names.getTypeId("High Energy Physics"),
        names.getTypeId("Hydromagnetic Physics"),
        names.getTypeId("Laser Physics"),
        names.getTypeId("Mechanical Engineering"),
        names.getTypeId("Molecular Engineering"),
        names.getTypeId("Nanite Engineering"),
        names.getTypeId("Nuclear Physics"),
        names.getTypeId("Plasma Physics"),
        names.getTypeId("Quantum Physics"),
        names.getTypeId("Rocket Science"),
        names.getTypeId("Offensive Subsystem Technology"),
        names.getTypeId("Defensive Subsystem Technology"),
        names.getTypeId("Core Subsystem Technology"),
        names.getTypeId("Propulsion Subsystem Technology"),
    };
    manufacturingTimeModifierSkills = std::unordered_set<std::size_t> {
        names.getTypeId("Advanced Small Ship Construction"),
        names.getTypeId("Advanced Medium Ship Construction"),
        names.getTypeId("Advanced Large Ship Construction"),
        names.getTypeId("Advanced Industrial Ship Construction")
    };
}

std::size_t Skills::getSkillLevel(std::size_t skill) const {
    if (overrideSkills)
        return 5;
    auto l = skillLevels.find(skill);
    return l != skillLevels.end() ? l->second : 0;
}
