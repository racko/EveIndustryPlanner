#include "libs/industry_resources/production/products.h"

#include "blueprint_efficiency.h"
#include "libs/industry_resources/resource_manager/stuff.h"
#include "libs/industry_resources/types/types.h"
#include "skills.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <job_values.h>
#include <market_data.h>
#include <names.h>
#include <settings.h>
#include <tech_level.h>
#include <yaml-cpp/yaml.h>

namespace {
Production::skills_t makeRequiredSkills(const YAML::Node& skillsNode) {
    Production::skills_t required_skills;
    required_skills.reserve(skillsNode.size());
    std::transform(
        skillsNode.begin(), skillsNode.end(), std::back_inserter(required_skills), [](const YAML::Node& skill) {
            return Production::skill_requirement_t{skill["typeID"].as<unsigned>(), skill["level"].as<unsigned>()};
        });
    return required_skills;
}

double getManufacturingTimeMod(const Production::skills_t& required_skills, const Skills& skills) {
    double manufacturingTimeMod = 1.0;
    for (const auto& [skillId, level] : required_skills) {
        if (skills.isAdvancedScienceSkill(skillId) || skills.isRacialEncryptionSkill(skillId) ||
            skills.isManufacturingTimeModifierScienceSkill(skillId))
            manufacturingTimeMod *= (1.0 - double(skills.getSkillLevel(skillId)) * 0.01);
    }
    return manufacturingTimeMod;
}

Production::materials_t makeMaterials(const YAML::Node& materialsNode) {
    Production::materials_t materials;
    materials.reserve(materialsNode.size());
    std::transform(materialsNode.begin(), materialsNode.end(), std::back_inserter(materials), [](const YAML::Node& m) {
        return Production::material_requirement_t{m["typeID"].as<unsigned>(), m["quantity"].as<unsigned>()};
    });
    return materials;
}

bool isPricable(const Production::materials_t& materials, const std::unordered_map<std::size_t, double>& avgPrices) {
    return std::none_of(materials.begin(), materials.end(), [&avgPrices](const Production::material_requirement_t& m) {
        return avgPrices.find(m.first) == avgPrices.end();
    });
}

} // namespace

void Products::addManufacturingData(const YAML::Node& manufacturing,
                                    const unsigned blueprintId,
                                    /*const Names& names*/ const std::unordered_map<std::size_t, double>& avgPrices,
                                    const Skills& skills) {
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
    const auto productId = productsNode[0]["typeID"].as<unsigned>();
    std::cout << "  Manufacture " << productId << "\n";
    const auto productQuantity = productsNode[0]["quantity"].as<unsigned>();
    // auto productName = names.checkName(productId);
    // if (!productName) {
    //    std::cout << "    ERROR: Unknown typeID\n";
    //    return;
    //}
    // std::cout << "    Product name: " << *productName << "\n";
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
    const auto t = timeNode.as<int>();
    const auto& skillsNode = manufacturing["skills"];
    if (!skillsNode.IsDefined()) {
        std::cout << "    ERROR: No skills listed\n";
        return;
    }
    auto required_skills = makeRequiredSkills(skillsNode);
    const auto manufacturingTimeMod =
        skills.manufacturingTimeReduction * getManufacturingTimeMod(required_skills, skills);
    std::cout << "    time: " << t << '\n';
    std::cout << "    Inputs: \n";
    auto materials = makeMaterials(materialsNode);
    if (!isPricable(materials, avgPrices)) {
        std::cout << "    ERROR: Could not find all material prices. Ignoring blueprint\n";
        return;
    }
    products_.emplace_back(blueprintId,
                           productId,
                           productQuantity,
                           std::move(materials),
                           std::move(required_skills),
                           std::chrono::duration<double>(manufacturingTimeMod * t));
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

void Products::filter(const Skills& skills) { filterBySkill(products_, skills); }

std::vector<Job::Ptr> Products::getManufacturingJobs(const Names& names,
                                                     const Settings& settings,
                                                     Stuff& stuff,
                                                     const Types& types,
                                                     const MarketData& market,
                                                     const JobValues& values) const {
    std::vector<Job::Ptr> jobs;
    const auto& bpes = stuff.bpes;
    jobs.reserve(bpes.size() * getProducts().size());
    const auto& bp_selection = settings.blueprints;
    const auto& facilities = settings.facilities;

    const auto& productionLineTime = stuff.productionLineTime;
    for (const auto& p : getProducts()) {
        const auto bp = p.getBlueprint();
        if (bp_selection.ignoreList.count(bp) > 0)
            continue;

        const auto& bpeList = types.isT1BP(bp) ? std::vector<BlueprintEfficiency>{BlueprintEfficiency(1.0, 1.0)} : bpes;
        for (const auto& bpe : bpeList) {
            const auto me = bpe.getMaterialEfficiency();
            const auto te = bpe.getTimeEfficiency();
            const auto pId = p.getProduct();
            const auto duration = te * std::chrono::duration<double>{p.getDuration()}.count();
            const auto& materials = p.getMaterials();
            Job::Resources inBPC, outBPC{{stuff.getResource(pId), p.getProducedQuantity()}};
            inBPC.reserve(materials.size() + 2);
            std::transform(materials.begin(), materials.end(), std::back_inserter(inBPC), [&stuff, me](const auto& m) {
                return Job::MaterialRequirement{stuff.getResource(m.first), std::ceil(me * double(m.second))};
            });
            inBPC.emplace_back(productionLineTime, duration);
            const auto techLevel = to_string(types.getBPTechLevel(bp));
            const auto bpPriceIt = market.avgPrices.find(bp);
            // cases:
            //   - no price (T2, T3, faction ... we want to produce from T2/T3 bpcs)
            //     bpo: no
            //     bpc: yes (planner_ will make sure that we have the bpc. We only add T2 and T3 bpcs)
            //   - price <= limit (cheap T1)
            //     bpo: yes
            //     bpc: yes
            //   - price > limit (expensive T1)
            //     bpo: no
            //     bpc: no (yes: we don't add the copy job and could let the planner_ figure out that we can't produce
            //     this ...)
            if (types.isT1BP(bp) && bpPriceIt != market.avgPrices.end() &&
                bpPriceIt->second <= bp_selection.bpoPriceLimit) {
                auto inBPO = inBPC;
                auto outBPO = outBPC;
                inBPO.emplace_back(stuff.getBPO(bp), duration);
                jobs.push_back(std::make_unique<Job>(std::move(inBPO),
                                                     std::move(outBPO),
                                                     -values.at(pId) * facilities.productionTax,
                                                     techLevel + "_Produce_BPO_" + std::to_string(pId),
                                                     "Produce " + names.getName(pId) + " from BPO"));
            }
            inBPC.emplace_back(stuff.getBPC(BlueprintWithEfficiency(bp, me, te)), 1);
            const auto bpeString = std::to_string(me) + "_" + std::to_string(te);
            jobs.push_back(std::make_unique<Job>(std::move(inBPC),
                                                 std::move(outBPC),
                                                 -values.at(pId) * facilities.productionTax,
                                                 techLevel + "_Produce_" + bpeString + "_BPC_" + std::to_string(pId),
                                                 "Produce " + names.getName(pId) + " from " + bpeString + "_BPC"));
        }
    }
    return jobs;
}
