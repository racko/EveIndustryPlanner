#include "libs/industry_resources/copying/copies.h"

#include "libs/industry_resources/job_values.h"
#include "libs/industry_resources/resource_manager/stuff.h"
#include "market_data.h"
#include "names.h"
#include "skills.h"
#include <algorithm>
#include <iostream>
#include <yaml-cpp/yaml.h>

void Copies::addCopyingData(const YAML::Node& copying,
                            const unsigned blueprintId,
                            /*const Names& names*/ const std::unordered_map<std::size_t, double>& avgPrices) {
    std::cout << "  Copying:\n";
    if (!copying.IsDefined()) {
        std::cout << "    Cannot be copied\n"; // not an error: T3 relics can only be researched
        return;
    }
    const auto bpPriceIt = avgPrices.find(blueprintId);
    if (bpPriceIt == avgPrices.end()) {
        std::cout << "    BPO cannot be bought\n";
        return;
    }
    const auto& copyTime = copying["time"];
    if (!copyTime.IsDefined()) {
        std::cout << "    ERROR: No copy time listed\n";
        return;
    }
    const auto t = copyTime.as<int>();
    // std::cout << "  " << blueprintId << ": " << names.getName(blueprintId) << " [COPY]\n";
    std::cout << "    Copy time: " << t << " (no inputs)\n";
    t1Copies_.emplace_back(blueprintId, std::chrono::seconds(t));
}

std::vector<Job::Ptr> Copies::getCopyJobs(const Names& names,
                                          Stuff& stuff,
                                          const MarketData& market,
                                          const JobValues& values,
                                          const double copyTimeReduction,
                                          const double copyTax,
                                          [[maybe_unused]] const double bpoPriceLimit) const {
    std::vector<Job::Ptr> jobs;
    jobs.reserve(t1Copies_.size());
    const auto& labTime = stuff.labTime;

    for (const auto& c : t1Copies_) {
        const auto bp = c.getBlueprint();
        const auto pId = c.getBlueprint();

        [[maybe_unused]] const auto bpPriceIt = market.avgPrices.find(bp);
        const auto jobValueIt = values.find(bp);
        assert(bpPriceIt != market.avgPrices.end() && bpPriceIt->second <= bpoPriceLimit);
        if (jobValueIt == values.end())
            continue;
        const auto jobValue = jobValueIt->second;
        const auto duration =
            copyTimeReduction * double(std::chrono::duration_cast<std::chrono::seconds>(c.getDuration()).count());

        const auto name = names.checkName(pId);
        assert(name); // guaranteed by addCopyingData

        jobs.push_back(
            std::make_unique<Job>(Job::InputResources{{stuff.getBPO(bp), duration}, {labTime, duration}},
                                  Job::OutputResources{{stuff.getBPC(BlueprintWithEfficiency(pId, 1, 1)), 1}},
                                  -jobValue * copyTax,
                                  "Copy_" + std::to_string(pId),
                                  "Copy " + *name));
    }
    return jobs;
}

namespace {
void filterCopies(Copies::copies_t& copies,
                  const std::unordered_map<std::size_t, double>& avgPrices,
                  const double bpoPriceLimit) {
    auto bpo_too_expensive = [&](const Copying& c) {
        auto bpPrice = avgPrices.find(c.getBlueprint());
        return bpPrice == avgPrices.end() || bpPrice->second > bpoPriceLimit;
    };
    copies.erase(std::remove_if(copies.begin(), copies.end(), bpo_too_expensive), copies.end());
}
} // namespace

void Copies::filter(const std::unordered_map<std::size_t, double>& avgPrices, const double bpoPriceLimit) {
    filterCopies(t1Copies_, avgPrices, bpoPriceLimit);
}
