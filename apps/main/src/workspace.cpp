#include "workspace.h"

#include "libs/industry_resources/input_values.h"
#include "libs/industry_resources/resource_manager/stuff.h"
#include "libs/profiling/profiling.h"
#include "libs/sqlite-util/sqlite.h"
#include "planner.h"
#include <boost/filesystem.hpp>
#include <yaml-cpp/yaml.h>

namespace {
YAML::Node loadYaml(const char* file_name) {
    std::cout << "loading " << file_name << ". " << std::flush;
    YAML::Node yaml_root;
    const auto bpTime = time<float, std::milli>([&] { yaml_root = YAML::LoadFile(file_name); });
    std::cout << bpTime.count() << "ms. Have " << yaml_root.size() << " entries\n";
    return yaml_root;
}

class PlannerAndStuff {
  public:
    PlannerAndStuff(const workspace& ws);
    void feedPlanner();

  private:
    void addPlanetary();
    void addMarket();
    void addProduction();
    void addT2Invention();
    void addT3Invention();
    void addCopy();

    const Settings& settings_;
    const Skills& skills_;
    const Assets& assets_;
    const MarketData& market_;
    const JobValues& values_;
    const Names& names_;
    const Groups& groups_;
    const Jobs& jobs_;
    const Types& types_;

    Stuff stuff_;
    Planner planner_;
};

void PlannerAndStuff::addPlanetary() {
    for (auto&& job :
         jobs_.getPlanetaryJobs(stuff_, assets_, names_, skills_.cpu, skills_.power, settings_.industry.timeFrame))
        planner_.addJob(std::move(job));
}

void PlannerAndStuff::addMarket() {
    const auto& trade = settings_.trade;
    for (auto&& job : market_.getMarketJobs(names_, stuff_, types_, trade.brokersFee, trade.salesTax))
        planner_.addJob(std::move(job));
}

void PlannerAndStuff::addProduction() {
    for (auto&& job : jobs_.getManufacturingJobs(names_, settings_, stuff_, types_, market_, values_))
        planner_.addJob(std::move(job));
}

void PlannerAndStuff::addT2Invention() {
    for (auto&& job : jobs_.getT2InventionJobs(
             names_, stuff_, settings_.facilities.inventionTax, values_, skills_.researchTimeReduction))
        planner_.addJob(std::move(job));
}

void PlannerAndStuff::addT3Invention() {
    for (auto&& job : jobs_.getT3InventionJobs(
             names_, stuff_, settings_.facilities.inventionTax, values_, skills_.researchTimeReduction))
        planner_.addJob(std::move(job));
}

void PlannerAndStuff::addCopy() {
    for (auto&& job : jobs_.getCopyJobs(names_,
                                        stuff_,
                                        market_,
                                        values_,
                                        skills_.copyTimeReduction,
                                        settings_.facilities.copyTax,
                                        settings_.blueprints.bpoPriceLimit))
        planner_.addJob(std::move(job));
}

PlannerAndStuff::PlannerAndStuff(const workspace& ws)
    : settings_{ws.getSettings()},
      skills_{ws.getSkills()},
      assets_{ws.getAssets()},
      market_{ws.getMarketData()},
      values_{ws.getJobValues()},
      names_{ws.getNames()},
      groups_{ws.getGroups()},
      jobs_{ws.getJobs()},
      types_{ws.getTypes()},
      stuff_{names_, assets_, types_, settings_.industry} {}

void PlannerAndStuff::feedPlanner() {
    std::cout << "feeding planner_\n";

    this->addPlanetary();
    this->addMarket();
    this->addProduction();
    this->addT2Invention();
    this->addT3Invention();
    this->addCopy();
    planner_.solve(settings_.industry.buyIn);
}

} // namespace

void workspace::loadTypeIds() {
    boost::filesystem::path names_path{"names.dat"};
    boost::filesystem::path groups_path{"groups.dat"};
    if (!boost::filesystem::exists(names_path) || !boost::filesystem::exists(groups_path)) {
        const YAML::Node typesNode = ::loadYaml("typeIDs.yaml"); // 7s

        TIME({ names_.loadNames(typesNode); });
        TIME({ groups_.loadGroups(typesNode); });

        {
            std::ofstream names_file{names_path};
            names_.serialize(names_file);
            std::ofstream groups_file{groups_path};
            groups_.serialize(groups_file);
        }
        {
            Names deserialized_names;
            Groups deserialized_groups;
            std::ifstream names_file{names_path};
            deserialized_names.deserialize(names_file);
            if (names_ != deserialized_names) {
                throw std::runtime_error("de-/serialization inconsistent");
            }
            std::ifstream groups_file{groups_path};
            deserialized_groups.deserialize(groups_file);
            if (groups_ != deserialized_groups) {
                throw std::runtime_error("de-/serialization inconsistent");
            }
        }
    } else {
        std::ifstream names_file{names_path};
        names_.deserialize(names_file);
        std::ifstream groups_file{groups_path};
        groups_.deserialize(groups_file);
    }
}

void workspace::loadMarketData() {
    boost::filesystem::path market_data_path{"market_data.dat"};
    if (!boost::filesystem::exists(market_data_path)) {
        TIME({
            market_.loadOrdersFromDB(
                sqlite::getDB("market_history.db"), settings_.queries.buyQuery, settings_.queries.sellQuery, names_);
        }); // 7s

        {
            std::ofstream market_data_file{market_data_path};
            market_.serialize(market_data_file);
        }
        {
            MarketData deserialized_market_data;
            std::ifstream market_data_file{market_data_path};
            deserialized_market_data.deserialize(market_data_file);
            if (market_.sellorders != deserialized_market_data.sellorders) {
                throw std::runtime_error("de-/serialization inconsistent");
            }
        }
    } else {
        std::ifstream market_data_file{market_data_path};
        market_.deserialize(market_data_file);
    }
}

workspace::workspace() {
    TIME({ settings_.read(); });

    loadTypeIds();
    TIME({ skills_.loadSkillLevels(names_); });
    settings_.industry.productionLines = std::min(settings_.industry.productionLines, skills_.productionLines);
    settings_.industry.scienceLabs = std::min(settings_.industry.scienceLabs, skills_.scienceLabs);
    TIME({ assets_.loadAssetsFromTsv(names_); });
    TIME({ assets_.loadOwnedBPCsFromXml(names_); });
    TIME({ market_.loadPrices(names_); });
    loadMarketData();
    TIME({ jobs_.loadSchematics(/*names_*/); });
    TIME({ this->processBlueprints(); });

    const auto iv = ::inputValues(jobs_.getProducts(), market_.adjPrices);

    // modifies jobs_. Has to happen between processBlueprints() and feedPlanner()
    jobs_.filter(skills_, market_.avgPrices, settings_.blueprints.bpoPriceLimit);

    types_.isConsistent();
    // TIME({ values_.setJobValues(jobs_.getInputValues(), types_.getBlueprints()); });
    TIME({ values_.setJobValues(iv, types_.getBlueprints()); });
    return;
    TIME({ this->feedPlanner(); }); // 13s
}

void workspace::processBlueprints() {
    const auto& blueprints = ::loadYaml("blueprints.yaml");
    const auto count = blueprints.size();
    jobs_.reserve(count);
    types_.reserve(count);
    for (const auto& b : blueprints) {
        const auto blueprintId = b.first.as<unsigned>();
        types_.registerBlueprint(blueprintId, b.second, groups_);
        std::cout << blueprintId;
        const auto blueprintName = names_.checkName(blueprintId);
        if (!blueprintName) {
            std::cout << ": unknown typeID\n";
            continue;
        }
        std::cout << " " << *blueprintName << ":\n";
        // auto bpPriceIt = market_.avgPrices.find(blueprintId);
        // if ((bpPriceIt != market_.avgPrices.end() && bpPriceIt->second > settings_.blueprints.bpoPriceLimit)) {
        //    std::cout << "  Ignored: too expensive\n";
        //    continue;
        //}

        const auto& activities = b.second["activities"];

        jobs_.addManufacturingData(activities["manufacturing"], blueprintId, /*names_,*/ market_.avgPrices, skills_);
        // TODO: filter manufacturing by names.check(productid)
        jobs_.addInventionData(activities["invention"], blueprintId, /*names_,*/ market_.avgPrices, skills_);
        jobs_.addCopyingData(activities["copying"], blueprintId, /*names_,*/ market_.avgPrices);
    }
    std::cout << "Have " << jobs_.getProducts().size() << " products\n";
}

void workspace::feedPlanner() {
    PlannerAndStuff x{*this};
    x.feedPlanner();
}
