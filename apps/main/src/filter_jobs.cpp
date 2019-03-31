#include "filter_jobs.h"

#include "jobs.h"
#include "skills.h"

#include <algorithm>

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

void filterCopies(Jobs::copies_t& copies, const std::unordered_map<std::size_t, double>& avgPrices, double bpoPriceLimit) {
    auto bpo_too_expensive = [&](const Copying& c) {
        auto bpPrice = avgPrices.find(c.getBlueprint());
        return bpPrice == avgPrices.end() || bpPrice->second > bpoPriceLimit;
    };
    copies.erase(std::remove_if(copies.begin(), copies.end(), bpo_too_expensive), copies.end());
}
} // namespace

void filter_jobs(Jobs& jobs, const Skills& skills, const std::unordered_map<std::size_t, double>& avgPrices,
                 double bpoPriceLimit) {
    filterBySkill(jobs.getProducts(), skills);
    filterCopies(jobs.getT1Copies(), avgPrices, bpoPriceLimit);
    filterBySkill(jobs.getT2Blueprints(), skills);
    filterBySkill(jobs.getT3Blueprints(), skills);
}
