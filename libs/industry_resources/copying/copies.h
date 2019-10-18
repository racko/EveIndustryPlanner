#pragma once

#include "libs/industry_resources/copying.h"
#include "libs/industry_resources/job.h"
#include "yaml_fwd.h"
#include <unordered_map>
#include <vector>

struct JobValues;
struct MarketData;
struct Names;
struct Skills;
struct Stuff;

struct Copies {
    using copies_t = std::vector<Copying>;

    void addCopyingData(const YAML::Node& copying,
                        unsigned blueprintId,
                        /*const Names& names*/ const std::unordered_map<std::size_t, double>& avgPrices);

    const copies_t& getT1Copies() const& { return t1Copies_; }
    void reserve(std::size_t n);

    std::vector<Job::Ptr> getCopyJobs(const Names& names,
                                      Stuff& stuff,
                                      const MarketData& market,
                                      const JobValues& values,
                                      const double copyTimeReduction,
                                      const double copyTax,
                                      const double bpoPriceLimit) const;

    void filter(const std::unordered_map<std::size_t, double>& avgPrices, double bpoPriceLimit);

  private:
    copies_t t1Copies_;
};
