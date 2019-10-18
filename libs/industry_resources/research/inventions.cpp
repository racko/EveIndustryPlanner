#include "libs/industry_resources/research/inventions.h"

#include "libs/industry_resources/job_values.h"
#include "libs/industry_resources/resource_manager/stuff.h"
#include "names.h"
#include "skills.h"
#include <algorithm>
#include <iostream>
#include <yaml-cpp/yaml.h>

void Inventions::addInventionData(const YAML::Node& invention,
                                  unsigned blueprintId,
                                  /*const Names& names*/ const std::unordered_map<std::size_t, double>& avgPrices,
                                  const Skills& skills) {
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
        } else if (skills.isAdvancedScienceSkill(skillId)) {
            scienceSkills.push_back(skillId);
            scienceSkillLevels.push_back(level);
        }
    }
    if (scienceSkills.size() != 2 || racialSkill.size() != 1) {
        std::cout << "    ERROR: wrong skill count.\n";
        return;
    }
    auto probMod =
        1. + (double(skills.getSkillLevel(scienceSkills[0])) + double(skills.getSkillLevel(scienceSkills[1]))) / 30. +
        double(skills.getSkillLevel(racialSkill[0])) / 40.0;
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
        // auto productName = names.getName(productId, unknown);
        // std::cout << "  " << productId << ": " << productName << '\n';
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
            // auto mName = names.getName(mId, unknown);
            // std::cout << "      " << mName << ": " << quantity << '\n';
            if (avgPrices.find(mId) == avgPrices.end()) {
                std::cout << "    ERROR: no price found. Ignoring blueprint\n";
                return;
            }
            materials.emplace_back(mId, quantity);
        }
        // sleeper encryption methods -> Tech 3
        (racialSkill[0] != 3408 ? t2Blueprints_ : t3Blueprints_)
            .emplace_back(blueprintId,
                          productId,
                          productQuantity,
                          std::move(materials),
                          std::move(required_skills),
                          std::chrono::seconds(t),
                          probability);
    }
}

std::vector<Job::Ptr> Inventions::getT2InventionJobs(const Names& names,
                                                     Stuff& stuff,
                                                     const double inventionTax,
                                                     const JobValues& values,
                                                     const double researchTimeReduction) const {
    const auto& decryptors = stuff.decryptors;
    std::vector<Job::Ptr> jobs;
    jobs.reserve(t2Blueprints_.size() * decryptors.size());
    const auto& labTime = stuff.labTime;

    for (const auto& r : t2Blueprints_) {
        auto bp = r.getBlueprint();
        auto pId = r.getProduct();
        for (const auto& decryptor : decryptors) {
            auto jobValueIt = values.find(pId);
            if (jobValueIt == values.end())
                continue;
            Job::Resources in, out;
            in.reserve(r.getMaterials().size() + 1);
            for (const auto& m : r.getMaterials()) {
                in.emplace_back(stuff.getResource(m.first), m.second);
            }
            if (const auto& d = decryptor.getDecryptor()) {
                in.emplace_back(stuff.getResource(*d), 1);
            }
            in.emplace_back(stuff.getBPC(BlueprintWithEfficiency(bp, 1, 1)), 1);
            in.emplace_back(labTime, researchTimeReduction * double(std::chrono::seconds{r.getDuration()}.count()));
            auto me = decryptor.getMaterialEfficiency();
            auto te = decryptor.getTimeEfficiency();
            out.emplace_back(stuff.getBPC(BlueprintWithEfficiency(pId, me, te)),
                             decryptor.getProbabilityModifier() * r.getBaseProbability() *
                                 double(r.getProducedQuantity() + decryptor.getRunModifier()));
            auto decryptorString = decryptor.getDecryptor() ? std::to_string(*decryptor.getDecryptor()) : "none";
            jobs.push_back(std::make_unique<Job>(
                std::move(in),
                std::move(out),
                -jobValueIt->second * inventionTax,
                "Research_" + std::to_string(bp) + "_" + decryptorString + "_" + std::to_string(pId),
                "Research " + names.getName(pId) +
                    (decryptor.getDecryptor() ? " with " + names.getName(*decryptor.getDecryptor())
                                              : " without a decryptor")));
        }
    }
    return jobs;
}

std::vector<Job::Ptr> Inventions::getT3InventionJobs(const Names& names,
                                                     Stuff& stuff,
                                                     const double inventionTax,
                                                     const JobValues& values,
                                                     const double researchTimeReduction) const {
    const auto& decryptors = stuff.decryptors;
    std::vector<Job::Ptr> jobs;
    jobs.reserve(t3Blueprints_.size() * decryptors.size());
    const auto& labTime = stuff.labTime;

    for (const auto& r : t3Blueprints_) {
        auto bp = r.getBlueprint();
        auto pId = r.getProduct();
        for (const auto& decryptor : decryptors) {
            auto jobValueIt = values.find(pId);
            if (jobValueIt == values.end())
                continue;
            Job::Resources in, out;
            in.reserve(r.getMaterials().size() + 1);
            for (const auto& m : r.getMaterials()) {
                in.emplace_back(stuff.getResource(m.first), m.second);
            }
            if (const auto& d = decryptor.getDecryptor()) {
                in.emplace_back(stuff.getResource(*d), 1);
            }
            in.emplace_back(stuff.getResource(bp), 1); // <===== HERE'S THE SINGLE DIFFERENCE!
            in.emplace_back(labTime, researchTimeReduction * double(std::chrono::seconds{r.getDuration()}.count()));
            auto me = decryptor.getMaterialEfficiency();
            auto te = decryptor.getTimeEfficiency();
            out.emplace_back(stuff.getBPC(BlueprintWithEfficiency(pId, me, te)),
                             decryptor.getProbabilityModifier() * r.getBaseProbability() *
                                 double(r.getProducedQuantity() + decryptor.getRunModifier()));
            auto decryptorString = decryptor.getDecryptor() ? std::to_string(*decryptor.getDecryptor()) : "none";
            jobs.push_back(std::make_unique<Job>(
                std::move(in),
                std::move(out),
                -jobValueIt->second * inventionTax,
                "Research_" + std::to_string(bp) + "_" + decryptorString + "_" + std::to_string(pId),
                "Research " + names.getName(pId) +
                    (decryptor.getDecryptor() ? " with " + names.getName(*decryptor.getDecryptor())
                                              : " without a decryptor")));
        }
    }
    return jobs;
}

namespace {
template <typename T>
void filterBySkill(std::vector<T>& jobs, const Skills& skills) {
    using skill_t = unsigned;
    using level_t = unsigned;
    using skill_requirement_t = std::pair<skill_t, level_t>;

    auto skill_too_low = [&](const skill_requirement_t& s) { return skills.getSkillLevel(s.first) < s.second; };

    auto any_skill_too_low = [skill_too_low](const auto& p) {
        const auto& required_skills = p.getSkills();
        return std::any_of(required_skills.begin(), required_skills.end(), skill_too_low);
    };

    jobs.erase(std::remove_if(jobs.begin(), jobs.end(), any_skill_too_low), jobs.end());
}
} // namespace

void Inventions::filter(const Skills& skills) {
    filterBySkill(t2Blueprints_, skills);
    filterBySkill(t3Blueprints_, skills);
}
