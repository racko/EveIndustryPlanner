#include "libs/industry_resources/schematics/schematics.h"

#include "libs/assets/assets.h"
#include "libs/industry_resources/resource_manager/stuff.h"
#include <boost/numeric/conversion/cast.hpp>
#include <names.h>
#include <sqlite.h>
#include <unordered_set>

void Schematics::loadSchematics(/*const Names& names*/) {
    const auto db = sqlite::getDB("/cifs/server/Media/Tim/Eve/SDE/sqlite-latest.sqlite");
    const auto stmt = sqlite::prepare(
        db,
        "select planetschematicstypemap.*, marketgroupid, cycletime from planetschematicstypemap natural join invtypes "
        "join invmarketgroups using (marketgroupid) join planetschematics using (schematicid) order by schematicid;");
    std::unordered_map<std::size_t, std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>> output;
    output.reserve(100);
    std::unordered_multimap<std::size_t, std::pair<std::size_t, std::size_t>> inputs;
    inputs.reserve(100);
    std::unordered_set<std::size_t> types;
    while (sqlite::step(stmt) == sqlite::ROW) {
        const auto schematicID = boost::numeric_cast<std::size_t>(sqlite::column<std::int32_t>(stmt, 0));
        const auto typeID = boost::numeric_cast<std::size_t>(sqlite::column<std::int32_t>(stmt, 1));
        const auto quantity = boost::numeric_cast<std::size_t>(sqlite::column<std::int32_t>(stmt, 2));
        const auto isInput = boost::numeric_cast<std::size_t>(sqlite::column<std::int32_t>(stmt, 3));
        const auto marketGroup = boost::numeric_cast<std::size_t>(sqlite::column<std::int32_t>(stmt, 4));
        const auto cycleTime = boost::numeric_cast<std::size_t>(sqlite::column<std::int32_t>(stmt, 5));
        if (isInput) {
            inputs.emplace(schematicID, std::make_pair(typeID, quantity));
        } else {
            const auto tier = marketGroup - 1334u;
            if (tier > 3)
                throw std::runtime_error("bad tier: " + std::to_string(tier));
            output.emplace(schematicID, std::make_tuple(typeID, quantity, tier, cycleTime));
        }
        types.insert(typeID);
    }
    this->typeCount = types.size();
    schematics_.reserve(output.size());
    for (const auto& s : output) {
        const auto ins = inputs.equal_range(s.first);
        Planetary::materials_t mats;
        mats.reserve(std::size_t(std::distance(ins.first, ins.second)));
        const auto product = std::get<0>(s.second);
        const auto quantity = std::get<1>(s.second);
        const auto duration = std::chrono::seconds{std::get<3>(s.second)};
        const auto tier = std::get<2>(s.second);
        // std::cout << quantity << " " << names.getName(product) << ", Tier " << tier << ", " << duration.count() << "
        // seconds: \n";
        for (auto i = ins.first; i != ins.second; ++i) {
            // std::cout << "  " << i->second.second << " " << names.getName(i->second.first) << '\n';
            mats.push_back(i->second);
        }
        schematics_.emplace_back(product, quantity, std::move(mats), duration, tier);
    }
}

namespace {
std::pair<double, unsigned> getMaterialFeeAndTier(const Schematics::schematics_t& schematics, const unsigned long id) {
    constexpr std::array<double, 5> base_value{{5.0, 400.0, 7200.0, 60e3, 1.2e6}};
    constexpr double poco_tax = 0.17;

    const auto feeIt =
        std::find_if(schematics.begin(), schematics.end(), [id](const Planetary& p) { return p.getProduct() == id; });
    const auto tier = feeIt == schematics.end() ? 0u : feeIt->getTier() + 1u;
    return std::pair{poco_tax * base_value[tier], tier};
}
} // namespace

std::vector<Job::Ptr> Schematics::getPlanetaryJobs(Stuff& stuff,
                                                   const Assets& assets,
                                                   const Names& names,
                                                   const double cpulimit,
                                                   const double powerlimit,
                                                   const double timeFrame) const {
    std::vector<Job::Ptr> jobs;
    jobs.reserve(3 + schematics_.size() + 2 * this->typeCount);
    const auto cpu = std::make_shared<Resource_Base>("CPU", "CPU", -cpulimit);
    const auto power = std::make_shared<Resource_Base>("Power", "Power", -powerlimit);
    const std::array<std::shared_ptr<Resource_Base>, 3> pi_time{
        std::make_shared<Resource_Base>("BasicIndustryTime", "Basic Industry Time"),
        std::make_shared<Resource_Base>("AdvancedIndustryTime", "Advanced Industry Time"),
        std::make_shared<Resource_Base>("HighTechIndustryTime", "High Tech Industry Time")};

    jobs.push_back(std::make_unique<Job>(Job::InputResources{{cpu, 200.0 + 22.0}, {power, 800.0 + 15.0}},
                                         Job::OutputResources{{pi_time[0], timeFrame}},
                                         0.0,
                                         "Build_Basic_Industry_Facility",
                                         "Build Basic Industry Facility"));

    jobs.push_back(std::make_unique<Job>(Job::InputResources{{cpu, 500.0 + 22.0}, {power, 700.0 + 15.0}},
                                         Job::OutputResources{{pi_time[1], timeFrame}},
                                         0.0,
                                         "Build_Advanced_Industry_Facility",
                                         "Build Advanced Industry Facility"));

    jobs.push_back(std::make_unique<Job>(Job::InputResources{{cpu, 1100.0 + 22.0}, {power, 400.0 + 15.0}},
                                         Job::OutputResources{{pi_time[2], timeFrame}},
                                         0.0,
                                         "Build_High_Tech_Production_Plant",
                                         "Build High Tech Production Plant"));

    const std::array<std::shared_ptr<Resource_Base>, 4> tier_times{{pi_time[0], pi_time[1], pi_time[1], pi_time[2]}};
    std::unordered_map<TypeID, Resource_Base::Ptr> planetaryItems;
    const auto& schematics = schematics_;

    const auto get_resource = [&stuff, &planetaryItems, &schematics, &assets, &jobs](const TypeID id,
                                                                                     const std::string& name) {
        auto& pResource = planetaryItems[id];
        if (pResource == nullptr) {
            const auto [fee, tier] = getMaterialFeeAndTier(schematics, id);
            const auto tierString = std::to_string(tier);
            const auto& resource = stuff.getResource(id);
            pResource = std::make_shared<Resource_Base>(
                "Planetary_" + std::to_string(id), "Planetary " + name, -double(assets.countPlanetaryAsset(id)));
            jobs.push_back(std::make_unique<Job>(Job::InputResources{{resource, 1.0}},
                                                 Job::OutputResources{{pResource, 1.0}},
                                                 -0.5 * fee,
                                                 "P" + tierString + "_Import_" + name,
                                                 "Import " + name + " (P" + tierString + ")"));
            jobs.push_back(std::make_unique<Job>(Job::InputResources{{pResource, 1.0}},
                                                 Job::OutputResources{{resource, 1.0}},
                                                 -fee,
                                                 "P" + tierString + "_Export_" + name,
                                                 "Export " + name + " (P" + tierString + ")"));
        }
        return pResource;
    };

    for (const auto& s : schematics) {
        Job::InputResources in;
        in.reserve(s.getMaterials().size() + 1);
        for (const auto& m : s.getMaterials()) {
            in.emplace_back(get_resource(m.first, names.getName(m.first)), m.second);
        }
        const auto tier = s.getTier();
        in.emplace_back(tier_times[tier], std::chrono::seconds{s.getDuration()}.count());
        const auto pId = s.getProduct();
        const auto& pName = names.getName(pId);
        const auto tierString = std::to_string(tier + 1);
        jobs.push_back(std::make_unique<Job>(std::move(in),
                                             Job::OutputResources{{get_resource(pId, pName), s.getProducedQuantity()}},
                                             0.0,
                                             "P" + tierString + "_Produce_" + pName,
                                             "Produce " + pName + " (P" + tierString + ")"));
    }
    return jobs;
}
