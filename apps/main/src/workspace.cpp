#include "workspace.h"

#include "buy.h"
#include "decryptor_property.h"
#include "filter_jobs.h"
#include "input_values.h"
#include "profiling.h"
#include "sell.h"
#include "sqlite.h"
#include "stuff.h"
#include "tech_level.h"

#include <yaml-cpp/yaml.h>

namespace {
YAML::Node loadBlueprints() {
    std::cout << "loading blueprints.yaml. " << std::flush;
    YAML::Node blueprints;
    auto bpTime = time<float, std::milli>([&] { blueprints = YAML::LoadFile("blueprints.yaml"); });
    std::cout << bpTime.count() << "ms. Have " << blueprints.size() << " blueprints\n";
    return blueprints;
}
} // namespace

workspace::workspace() {
    TIME({ settings_.read(); });

    // 7s
    std::cout << "loading typeIDs.yaml. " << std::flush;
    YAML::Node typesNode;
    auto tTime = time<float, std::milli>([&] { typesNode = YAML::LoadFile("typeIDs.yaml"); });
    std::cout << tTime.count() << "ms. Have " << typesNode.size() << " types\n";

    TIME({ names_.loadNames(typesNode); });
    TIME({ groups_.loadGroups(typesNode); });
    TIME({ skills_.loadSkillLevels(names_); });
    settings_.industry.productionLines = std::min(settings_.industry.productionLines, skills_.productionLines);
    settings_.industry.scienceLabs = std::min(settings_.industry.scienceLabs, skills_.scienceLabs);
    TIME({ assets_.loadAssetsFromTsv(names_); });
    TIME({ assets_.loadOwnedBPCsFromXml(names_); });
    TIME({ market_.loadPrices(names_); });
    TIME({
        market_.loadOrdersFromDB(sqlite::getDB("market_history.db"), settings_.queries.buyQuery,
                                 settings_.queries.sellQuery, names_);
    }); // 7s
    TIME({ jobs_.loadSchematics(names_); });
    TIME({ processBlueprints(); });

    auto iv = inputValues(jobs_.getProducts(), market_.adjPrices);

    // modifies jobs_. Has to happen between processBlueprints() and feedPlanner()
    filter_jobs(jobs_, skills_, market_.avgPrices, settings_.blueprints.bpoPriceLimit);

    types_.isConsistent();
    // TIME({ values_.setJobValues(jobs_.getInputValues(), types_.getBlueprints()); });
    TIME({ values_.setJobValues(iv, types_.getBlueprints()); });
    TIME({ feedPlanner(); }); // 13s
}

void workspace::processBlueprints() {
    const auto& blueprints = loadBlueprints();
    auto count = blueprints.size();
    // auto count = size_t(std::distance(blueprints.begin(), blueprints.end()));
    jobs_.reserve(count);
    types_.reserve(count);
    for (const auto& b : blueprints) {
        auto blueprintId = b.first.as<unsigned>();
        types_.registerBlueprint(blueprintId, b.second, groups_);
        std::cout << blueprintId;
        auto blueprintName = names_.checkName(blueprintId);
        if (!blueprintName) {
            std::cout << ": unknown typeID\n";
            continue;
        }
        std::cout << " " << *blueprintName;
        std::cout << ":\n";
        // auto bpPriceIt = market_.avgPrices.find(blueprintId);
        // if ((bpPriceIt != market_.avgPrices.end() && bpPriceIt->second > settings_.blueprints.bpoPriceLimit)) {
        //    std::cout << "  Ignored: too expensive\n";
        //    continue;
        //}

        const auto& activities = b.second["activities"];

        jobs_.addManufacturingData(activities["manufacturing"], blueprintId, names_, market_.avgPrices, skills_);
        jobs_.addInventionData(activities["invention"], blueprintId, names_, market_.avgPrices, skills_);
        jobs_.addCopyingData(activities["copying"], blueprintId, names_, market_.avgPrices);
    }
    std::cout << "Have " << jobs_.getProducts().size() << " products\n";
}

void workspace::addPlanetary(Stuff& stuff) {
    const auto& industry = settings_.industry;
    if (!industry.do_planetary_interaction)
        return;
    auto cpu = std::make_shared<Resource_Base>("CPU", "CPU", -skills_.cpu);
    auto power = std::make_shared<Resource_Base>("Power", "Power", -skills_.power);
    std::array<std::shared_ptr<Resource_Base>, 3> pi_time;
    pi_time[0] = std::make_shared<Resource_Base>("BasicIndustryTime", "Basic Industry Time");
    pi_time[1] = std::make_shared<Resource_Base>("AdvancedIndustryTime", "Advanced Industry Time");
    pi_time[2] = std::make_shared<Resource_Base>("HighTechIndustryTime", "High Tech Industry Time");

    {
        Job::Resources tier0in{{cpu, 200.0 + 22.0}, {power, 800.0 + 15.0}};
        Job::Resources tier0out{{pi_time[0], industry.timeFrame}};
        planner.addJob(std::make_shared<Job>(std::move(tier0in), std::move(tier0out), 0.0,
                                             "Build_Basic_Industry_Facility", "Build Basic Industry Facility"));
    }

    {
        Job::Resources tier1in{{cpu, 500.0 + 22.0}, {power, 700.0 + 15.0}};
        Job::Resources tier1out{{pi_time[1], industry.timeFrame}};
        planner.addJob(std::make_shared<Job>(std::move(tier1in), std::move(tier1out), 0.0,
                                             "Build_Advanced_Industry_Facility", "Build Advanced Industry Facility"));
    }

    {
        Job::Resources tier2in{{cpu, 1100.0 + 22.0}, {power, 400.0 + 15.0}};
        Job::Resources tier2out{{pi_time[2], industry.timeFrame}};
        planner.addJob(std::make_shared<Job>(std::move(tier2in), std::move(tier2out), 0.0,
                                             "Build_High_Tech_Production_Plant", "Build High Tech Production Plant"));
    }

    std::array<std::shared_ptr<Resource_Base>, 4> tier_times{{pi_time[0], pi_time[1], pi_time[1], pi_time[2]}};
    std::unordered_map<TypeID, Resource_Base::Ptr> planetaryItems;
    std::array<double, 5> base_value{{5.0, 400.0, 7200.0, 60e3, 1.2e6}};
    double poco_tax = 0.17;
    const auto& schematics = jobs_.getSchematics();
    for (const auto& s : schematics) {
        auto pId = s.getProduct();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(s.getDuration()).count();
        auto tier = s.getTier();
        Job::Resources in, out;
        in.reserve(s.getMaterials().size() + 2);
        {
            const auto& resource = stuff.getResource(pId);

            auto& pResource = planetaryItems[pId];
            if (pResource == nullptr) {
                pResource = std::make_shared<Resource_Base>("Planetary_" + std::to_string(pId),
                                                            "Planetary " + names_.getName(pId),
                                                            -double(assets_.countPlanetaryAsset(pId)));
                auto fee = poco_tax * base_value[tier + 1u];
                planner.addJob(
                    std::make_shared<Job>(Job::Resources{{resource, 1.0}}, Job::Resources{{pResource, 1.0}}, -0.5 * fee,
                                          "P" + std::to_string(tier + 1) + "_Import_" + names_.getName(pId),
                                          "Import " + names_.getName(pId) + " (P" + std::to_string(tier + 1) + ")"));
                planner.addJob(
                    std::make_shared<Job>(Job::Resources{{pResource, 1.0}}, Job::Resources{{resource, 1.0}}, -fee,
                                          "P" + std::to_string(tier + 1) + "_Export_" + names_.getName(pId),
                                          "Export " + names_.getName(pId) + " (P" + std::to_string(tier + 1) + ")"));
            }

            out.emplace_back(pResource, s.getProducedQuantity());
        }
        for (const auto& m : s.getMaterials()) {
            const auto& resource = stuff.getResource(m.first);
            auto& pResource = planetaryItems[m.first];
            if (pResource == nullptr) {
                pResource = std::make_shared<Resource_Base>("Planetary_" + std::to_string(m.first),
                                                            "Planetary " + names_.getName(m.first),
                                                            -double(assets_.countPlanetaryAsset(m.first)));
                auto feeIt = std::find_if(schematics.begin(), schematics.end(),
                                          [&](const Planetary& p) { return p.getProduct() == m.first; });
                double fee = poco_tax;
                auto mtier = 0u;
                if (feeIt == schematics.end())
                    fee *= base_value[0];
                else {
                    mtier = feeIt->getTier();
                    fee *= base_value[mtier + 1];
                }
                planner.addJob(std::make_shared<Job>(
                    Job::Resources{{resource, 1.0}}, Job::Resources{{pResource, 1.0}}, -0.5 * fee,
                    "P" + std::to_string(mtier + 1) + "_Import_" + names_.getName(m.first),
                    "Import " + names_.getName(m.first) + " (P" + std::to_string(mtier + 1) + ")"));
                planner.addJob(std::make_shared<Job>(
                    Job::Resources{{pResource, 1.0}}, Job::Resources{{resource, 1.0}}, -fee,
                    "P" + std::to_string(mtier + 1) + "_Export_" + names_.getName(m.first),
                    "Export " + names_.getName(m.first) + " (P" + std::to_string(mtier + 1) + ")"));
            }
            in.emplace_back(pResource, m.second);
        }
        in.emplace_back(tier_times[tier], duration);
        planner.addJob(std::make_shared<Job>(
            std::move(in), std::move(out), 0.0, "P" + std::to_string(tier + 1) + "_Produce_" + names_.getName(pId),
            "Produce " + names_.getName(pId) + " (P" + std::to_string(tier + 1) + ")"));
    }
}

void workspace::addMarket(Stuff& stuff) {
    const auto& trade = settings_.trade;
    std::cout << "  adding sell jobs\n";
    for (const auto& s : market_.sellorders) {
        assert(names_.checkName(s.type));
        if (types_.isBP(s.type))
            continue;
        planner.addJob(std::make_shared<Buy>(stuff.getResource(s.type), s.id, -s.price * (1.0 + trade.brokersFee),
                                             double(s.volume), s.stationID, s.stationName));
    }
    std::cout << "  adding buy jobs\n";
    for (const auto& b : market_.buyorders) {
        assert(names_.checkName(b.type));
        if (types_.isBP(b.type))
            continue;
        planner.addJob(std::make_shared<Sell>(stuff.getResource(b.type), b.id,
                                              b.price * (1.0 - trade.brokersFee - trade.salesTax), double(b.volume),
                                              b.stationID, b.stationName));
    }
}

void workspace::addProduction(Stuff& stuff) {
    const auto& bp_selection = settings_.blueprints;
    const auto& facilities = settings_.facilities;

    const auto& bpes = stuff.bpes;
    const auto& productionLineTime = stuff.productionLineTime;
    for (const auto& p : jobs_.getProducts()) {
        auto bp = p.getBlueprint();
        if (bp_selection.ignoreList.count(bp) > 0)
            continue;

        const auto& bpeList =
            types_.isT1BP(bp) ? std::vector<BlueprintEfficiency>{BlueprintEfficiency(1.0, 1.0)} : bpes;
        for (const auto& bpe : bpeList) {
            auto me = bpe.getMaterialEfficiency();
            auto te = bpe.getTimeEfficiency();
            auto pId = p.getProduct();
            auto duration = te * std::chrono::duration_cast<std::chrono::duration<double>>(p.getDuration()).count();
            Job::Resources inBPC, outBPC;
            inBPC.reserve(p.getMaterials().size() + 2);
            outBPC.emplace_back(stuff.getResource(pId), p.getProducedQuantity());
            for (const auto& m : p.getMaterials()) {
                inBPC.emplace_back(stuff.getResource(m.first), std::ceil(me * double(m.second)));
            }
            inBPC.emplace_back(productionLineTime, duration);
            auto techLevel = to_string(types_.getBPTechLevel(bp));
            auto bpPriceIt = market_.avgPrices.find(bp);
            // cases:
            //   - no price (T2, T3, faction ... we want to produce from T2/T3 bpcs)
            //     bpo: no
            //     bpc: yes (planner will make sure that we have the bpc. We only add T2 and T3 bpcs)
            //   - price <= limit (cheap T1)
            //     bpo: yes
            //     bpc: yes
            //   - price > limit (expensive T1)
            //     bpo: no
            //     bpc: no (yes: we don't add the copy job and could let the planner figure out that we can't produce
            //     this ...)
            if (types_.isT1BP(bp) && bpPriceIt != market_.avgPrices.end() &&
                bpPriceIt->second <= bp_selection.bpoPriceLimit) {
                auto inBPO = inBPC;
                auto outBPO = outBPC;
                inBPO.emplace_back(stuff.getBPO(bp), duration);
                planner.addJob(std::make_shared<Job>(
                    std::move(inBPO), std::move(outBPO), -values_.jobValues.at(pId) * facilities.productionTax,
                    techLevel + "_Produce_BPO_" + std::to_string(pId), "Produce " + names_.getName(pId) + " from BPO"));
            }
            inBPC.emplace_back(stuff.getBPC(BlueprintWithEfficiency(bp, me, te)), 1);
            auto bpeString = std::to_string(me) + "_" + std::to_string(te);
            planner.addJob(std::make_shared<Job>(std::move(inBPC), std::move(outBPC),
                                                 -values_.jobValues.at(pId) * facilities.productionTax,
                                                 techLevel + "_Produce_" + bpeString + "_BPC_" + std::to_string(pId),
                                                 "Produce " + names_.getName(pId) + " from " + bpeString + "_BPC"));
        }
    }
}

void workspace::addT2Invention(Stuff& stuff) {
    const auto& decryptors = stuff.decryptors;
    const auto& facilities = settings_.facilities;
    const auto& labTime = stuff.labTime;

    for (const auto& r : jobs_.getT2Blueprints()) {
        auto bp = r.getBlueprint();
        auto pId = r.getProduct();
        for (const auto& decryptor : decryptors) {
            auto jobValueIt = values_.jobValues.find(pId);
            if (jobValueIt == values_.jobValues.end())
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
            in.emplace_back(labTime,
                            skills_.researchTimeReduction *
                                double(std::chrono::duration_cast<std::chrono::seconds>(r.getDuration()).count()));
            auto me = decryptor.getMaterialEfficiency();
            auto te = decryptor.getTimeEfficiency();
            out.emplace_back(stuff.getBPC(BlueprintWithEfficiency(pId, me, te)),
                             decryptor.getProbabilityModifier() * r.getBaseProbability() *
                                 double(r.getProducedQuantity() + decryptor.getRunModifier()));
            auto decryptorString = decryptor.getDecryptor() ? std::to_string(*decryptor.getDecryptor()) : "none";
            planner.addJob(std::make_shared<Job>(
                std::move(in), std::move(out), -jobValueIt->second * facilities.inventionTax,
                "Research_" + std::to_string(bp) + "_" + decryptorString + "_" + std::to_string(pId),
                "Research " + names_.getName(pId) +
                    (decryptor.getDecryptor() ? " with " + names_.getName(*decryptor.getDecryptor())
                                              : " without a decryptor")));
        }
    }
}

void workspace::addT3Invention(Stuff& stuff) {
    const auto& decryptors = stuff.decryptors;
    const auto& facilities = settings_.facilities;
    const auto& labTime = stuff.labTime;

    for (const auto& r : jobs_.getT3Blueprints()) {
        auto bp = r.getBlueprint();
        auto pId = r.getProduct();
        for (const auto& decryptor : decryptors) {
            auto jobValueIt = values_.jobValues.find(pId);
            if (jobValueIt == values_.jobValues.end())
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
            in.emplace_back(labTime,
                            skills_.researchTimeReduction *
                                double(std::chrono::duration_cast<std::chrono::seconds>(r.getDuration()).count()));
            auto me = decryptor.getMaterialEfficiency();
            auto te = decryptor.getTimeEfficiency();
            out.emplace_back(stuff.getBPC(BlueprintWithEfficiency(pId, me, te)),
                             decryptor.getProbabilityModifier() * r.getBaseProbability() *
                                 double(r.getProducedQuantity() + decryptor.getRunModifier()));
            auto decryptorString = decryptor.getDecryptor() ? std::to_string(*decryptor.getDecryptor()) : "none";
            planner.addJob(std::make_shared<Job>(
                std::move(in), std::move(out), -jobValueIt->second * facilities.inventionTax,
                "Research_" + std::to_string(bp) + "_" + decryptorString + "_" + std::to_string(pId),
                "Research " + names_.getName(pId) +
                    (decryptor.getDecryptor() ? " with " + names_.getName(*decryptor.getDecryptor())
                                              : " without a decryptor")));
        }
    }
}

void workspace::addCopy(Stuff& stuff) {
    const auto& facilities = settings_.facilities;
    const auto& labTime = stuff.labTime;

    for (const auto& c : jobs_.getT1Copies()) {
        auto bp = c.getBlueprint();
        auto pId = c.getBlueprint();
        auto bpPriceIt = market_.avgPrices.find(bp);
        (void)bpPriceIt;
        auto jobValueIt = values_.jobValues.find(bp);
        auto name = names_.checkName(pId);
        assert(name); // guaranteed by addCopyingData
        assert(bpPriceIt != market_.avgPrices.end() && bpPriceIt->second <= settings_.blueprints.bpoPriceLimit);
        if (jobValueIt == values_.jobValues.end())
            continue;
        auto jobValue = jobValueIt->second;
        Job::Resources in, out;
        auto duration = skills_.copyTimeReduction *
                        double(std::chrono::duration_cast<std::chrono::seconds>(c.getDuration()).count());
        out.emplace_back(stuff.getBPC(BlueprintWithEfficiency(pId, 1, 1)), 1);
        in.emplace_back(stuff.getBPO(bp), duration);
        in.emplace_back(labTime, duration);
        planner.addJob(std::make_shared<Job>(std::move(in), std::move(out), -jobValue * facilities.copyTax,
                                             "Copy_" + std::to_string(pId), "Copy " + *name));
    }
}

void workspace::workspace::feedPlanner() {
    std::cout << "feeding planner\n";

    Stuff stuff(names_, assets_, types_, settings_.industry);
    addPlanetary(stuff);
    addMarket(stuff);
    addProduction(stuff);
    addT2Invention(stuff);
    addT3Invention(stuff);
    addCopy(stuff);
    planner.getLP(settings_.industry.buyIn);
}
