#pragma once

#include "libs/industry_resources/job.h"
#include "libs/industry_resources/production.h"
#include "yaml_fwd.h"
#include <unordered_map>
#include <vector>

struct JobValues;
struct MarketData;
struct Names;
struct Settings;
struct Skills;
struct Stuff;
struct Types;

struct Products {
    using products_t = std::vector<Production>;

    void addManufacturingData(const YAML::Node& manufacturing,
                              unsigned blueprintId,
                              /*const Names& names*/ const std::unordered_map<std::size_t, double>& avgPrices,
                              const Skills& skills);

    const products_t& getProducts() const& { return products_; }
    void reserve(std::size_t n);

    void filter(const Skills& skills);

    std::vector<Job::Ptr> getManufacturingJobs(const Names& names,
                                               const Settings& settings,
                                               Stuff& stuff,
                                               const Types& types,
                                               const MarketData& market,
                                               const JobValues& values) const;

  private:
    products_t products_;
};
