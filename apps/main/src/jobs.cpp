#include "jobs.h"

#include "names.h"
#include "skills.h"
#include <sqlite.h>

#include <boost/numeric/conversion/cast.hpp>
#include <iostream>
#include <yaml-cpp/yaml.h>

void Jobs::reserve(std::size_t) {

}

void Jobs::loadSchematics(const Names& names) {
    auto db = sqlite::getDB("/cifs/server/Media/Tim/Eve/SDE/sqlite-latest.sqlite");
    auto stmt = sqlite::prepare(db, "select planetschematicstypemap.*, marketgroupid, cycletime from planetschematicstypemap natural join invtypes join invmarketgroups using (marketgroupid) join planetschematics using (schematicid) order by schematicid;");
    std::unordered_map<std::size_t,std::tuple<std::size_t,std::size_t,std::size_t,std::size_t>> output;
    output.reserve(100);
    std::unordered_multimap<std::size_t,std::pair<std::size_t,std::size_t>> inputs;
    inputs.reserve(100);
    while(sqlite::step(stmt) == sqlite::ROW) {
        auto schematicID = boost::numeric_cast<std::size_t>(sqlite::column<int32_t>(stmt, 0));
        auto typeID = boost::numeric_cast<std::size_t>(sqlite::column<int32_t>(stmt, 1));
        auto quantity = boost::numeric_cast<std::size_t>(sqlite::column<int32_t>(stmt, 2));
        auto isInput = boost::numeric_cast<std::size_t>(sqlite::column<int32_t>(stmt, 3));
        auto marketGroup = boost::numeric_cast<std::size_t>(sqlite::column<int32_t>(stmt, 4));
        auto cycleTime = boost::numeric_cast<std::size_t>(sqlite::column<int32_t>(stmt, 5));
        if (!isInput) {
            output.emplace(schematicID, std::make_tuple(typeID, quantity, marketGroup - 1334u, cycleTime));
        } else {
            inputs.emplace(schematicID, std::make_pair(typeID, quantity));
        }
    }
    schematics.reserve(output.size());
    for (const auto& s : output) {
        auto ins = inputs.equal_range(s.first);
        // product_t product_, std::size_t producedQuantity_, materials_t materials_, duration_t duration_, tier_t tier_
        Planetary::materials_t mats;
        mats.reserve(std::size_t(std::distance(ins.first, ins.second)));
        auto product = std::get<0>(s.second);
        auto quantity = std::get<1>(s.second);
        auto duration = std::chrono::seconds{std::get<3>(s.second)};
        auto tier = std::get<2>(s.second);
        std::cout << quantity << " " << names.getName(product) << ", Tier " << tier << ", " << duration.count() << " seconds: \n";
        for (auto i = ins.first; i != ins.second; ++i) {
            std::cout << "  " << i->second.second << " " << names.getName(i->second.first) << '\n';
            mats.push_back(i->second);
        }
        schematics.emplace_back(product, quantity, std::move(mats), duration, tier);
    }
}

void Jobs::addManufacturingData(const YAML::Node& manufacturing, unsigned blueprintId, const Names& names, const std::unordered_map<std::size_t, double>& avgPrices, const Skills& skills) {
    if (!manufacturing.IsDefined()) {
        std::cout << "  Cannot be manufactured\n"; // not an error: T3 relics can only be researched
        return;
    }
    const auto& productsNode = manufacturing["products"];
    if (!productsNode.IsDefined()) {
        std::cout << "  ERROR: No products listed\n";
        return;
    }
    if (productsNode.size() != 1) {
        std::cout << "  ERROR: More or less than one manufacturing product\n";
        return;
    }
    auto productId = productsNode[0]["typeID"].as<unsigned>();
    std::cout <<  "  Manufacture " << productId << "\n";
    auto productQuantity = productsNode[0]["quantity"].as<unsigned>();
    auto productName = names.checkName(productId);
    if (!productName) {
        std::cout << "    ERROR: Unknown typeID\n";
        return;
    }
    std::cout << "    Product name: " << *productName << "\n";
    const auto& materialsNode = manufacturing["materials"];
    if (!materialsNode.IsDefined()) {
        std::cout << "    ERROR: No materials listed\n";
        return;
    }
    const auto& timeNode = manufacturing["time"];
    if (!timeNode.IsDefined()) {
        std::cout << "    ERROR: No time listed\n";
        return;
    }
    auto t = timeNode.as<int>();
    const auto& skillsNode = manufacturing["skills"];
    if (!skillsNode.IsDefined()) {
        std::cout << "    ERROR: No skills listed\n";
        return;
    }
    auto manufacturingTimeMod = skills.manufacturingTimeReduction;
    Production::skills_t required_skills;
    required_skills.reserve(skillsNode.size());
    for (const auto& skill : skillsNode) {
        auto skillId = skill["typeID"].as<unsigned>();
        auto level = skill["level"].as<unsigned>();
        required_skills.emplace_back(skillId, level);
        if (skills.isAdvancedScienceSkill(skillId) || skills.isRacialEncryptionSkill(skillId) || skills.isManufacturingTimeModifierScienceSkill(skillId))
            manufacturingTimeMod *= (1.0 - double(skills.getSkillLevel(skillId)) * 0.01);
    }
    std::cout << "    time: " << t << '\n';
    std::cout << "    Inputs: \n";
    auto materialCount = materialsNode.size();
    Production::materials_t materials;
    materials.reserve(materialCount);
    for (const auto& m : materialsNode) {
        auto mId = m["typeID"].as<unsigned>();
        auto quantity = m["quantity"].as<unsigned>();
        auto mNamePtr = names.checkName(mId);
        const auto& mName = mNamePtr ? *mNamePtr : "(unknown typeID)";
        std::cout << "      " << mName << ": " << quantity << '\n';
        if (avgPrices.find(mId) == avgPrices.end()) {
            std::cout << "    ERROR: no price found. Ignoring blueprint\n";
            return;
        }
        materials.emplace_back(mId, quantity);
    }
    products.emplace_back(blueprintId, productId, productQuantity, std::move(materials), std::move(required_skills), std::chrono::duration<double>(manufacturingTimeMod * t));
}

void Jobs::addInventionData(const YAML::Node& invention, unsigned blueprintId, const Names& names, const std::unordered_map<std::size_t, double>& avgPrices, const Skills& skills) {
    std::cout << "  Invention:\n";
    if (!invention.IsDefined()) {
        std::cout << "    Cannot be invented\n";
        return;
    }
    const auto& productsNode = invention["products"];
    if (!productsNode.IsDefined()) {
        std::cout << "    ERROR: No invention products listed\n";
        return;
    }
    const auto& skillsNode = invention["skills"];
    if (!skillsNode.IsDefined()) {
        std::cout << "    ERROR: No skills listed\n";
        return;
    }
    std::vector<std::size_t> scienceSkills;
    std::vector<std::size_t> scienceSkillLevels;
    scienceSkills.reserve(2);
    scienceSkillLevels.reserve(2);
    std::vector<std::size_t> racialSkillLevel;
    std::vector<std::size_t> racialSkill;
    racialSkill.reserve(1);
    racialSkillLevel.reserve(1);
    Research::skills_t required_skills;
    required_skills.reserve(skillsNode.size());
    for (const auto& skill : skillsNode) {
        auto skillId = skill["typeID"].as<unsigned>();
        auto level = skill["level"].as<unsigned>();
        required_skills.emplace_back(skillId, level);
        if (skills.isRacialEncryptionSkill(skillId)) {
            racialSkill.push_back(skillId);
            racialSkillLevel.push_back(level);
        }
        else if (skills.isAdvancedScienceSkill(skillId)) {
            scienceSkills.push_back(skillId);
            scienceSkillLevels.push_back(level);
        }
    }
    if (scienceSkills.size() != 2 || racialSkill.size() != 1) {
        std::cout << "    ERROR: wrong skill count.\n";
        return;
    }
    auto probMod = 1. + (double(skills.getSkillLevel(scienceSkills[0])) + double(skills.getSkillLevel(scienceSkills[1]))) / 30. + double(skills.getSkillLevel(racialSkill[0])) / 40.0;
    static const std::string unknown = "(unknown typeID)";
    for (const auto& productNode : productsNode) {
        auto productId = productNode["typeID"].as<unsigned>();
        auto productQuantity = productNode["quantity"].as<unsigned>();
        auto baseProbabilityNode = productNode["probability"];
        if (!baseProbabilityNode.IsDefined()) {
            std::cout << "    ERROR: No probability listed\n";
            continue;
        }
        auto baseProbability = baseProbabilityNode.as<double>();
        auto probability = baseProbability * probMod;
        auto productName = names.getName(productId, unknown);
        std::cout << "  " << productId << ": " << productName << '\n';
        const auto& materialsNode = invention["materials"];
        if (!materialsNode.IsDefined()) {
            std::cout << "    ERROR: No materials listed\n";
            continue;
        }
        const auto& timeNode = invention["time"];
        if (!timeNode.IsDefined()) {
            std::cout << "    ERROR: No time listed\n";
            continue;
        }
        auto t = timeNode.as<int>();
        std::cout << "    time: " << t << '\n';
        std::cout << "    base probability: " << baseProbability << '\n';
        std::cout << "    probability: " << probability << '\n';
        std::cout << "    Inputs: \n";
        auto materialCount = materialsNode.size();
        Research::materials_t materials;
        materials.reserve(materialCount);
        for (const auto& m : materialsNode) {
            auto mId = m["typeID"].as<unsigned>();
            auto quantity = m["quantity"].as<unsigned>();
            auto mName = names.getName(mId, unknown);
            std::cout << "      " << mName << ": " << quantity << '\n';
            if (avgPrices.find(mId) == avgPrices.end()) {
                std::cout << "    ERROR: no price found. Ignoring blueprint\n";
                return;
            }
            materials.emplace_back(mId, quantity);
        }
        if (racialSkill[0] != 3408) // sleeper encryption methods -> Tech 3
            t2Blueprints.emplace_back(blueprintId, productId, productQuantity, std::move(materials), std::move(required_skills), std::chrono::seconds(t), probability);
        else
            t3Blueprints.emplace_back(blueprintId, productId, productQuantity, std::move(materials), std::move(required_skills), std::chrono::seconds(t), probability);
    }
}

void Jobs::addCopyingData(const YAML::Node& copying, unsigned blueprintId, const Names& names, const std::unordered_map<std::size_t, double>& avgPrices) {
    std::cout << "  Copying:\n";
    if (!copying.IsDefined()) {
        std::cout << "    Cannot be copied\n"; // not an error: T3 relics can only be researched
        return;
    }
    auto bpPriceIt = avgPrices.find(blueprintId);
    if (bpPriceIt == avgPrices.end()) {
        std::cout << "    BPO cannot be bought\n";
        return;
    }
    const auto& copyTime = copying["time"];
    if (!copyTime.IsDefined()) {
        std::cout << "    ERROR: No copy time listed\n";
        return;
    }
    auto t = copyTime.as<int>();
    std::cout << "  " << blueprintId << ": " << names.getName(blueprintId) << " [COPY]\n";
    std::cout << "    Copy time: " << t << " (no inputs)\n";
    t1Copies.emplace_back(blueprintId, std::chrono::seconds(t));
}
