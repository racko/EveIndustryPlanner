#pragma once

#include "libs/industry_resources/job.h"
#include "libs/industry_resources/research.h"
#include "libs/yaml_fwd/yaml_fwd.h"
#include <unordered_map>
#include <vector>

struct JobValues;
struct Names;
struct Skills;
struct Stuff;

struct Inventions {
    using research_t = std::vector<Research>;

    void addInventionData(const YAML::Node& invention,
                          unsigned blueprintId,
                          /*const Names& names*/ const std::unordered_map<std::size_t, double>& avgPrices,
                          const Skills& skills);

    const research_t& getT2Blueprints() const& { return t2Blueprints_; }
    const research_t& getT3Blueprints() const& { return t3Blueprints_; }
    void reserve(std::size_t n);

    std::vector<Job::Ptr> getT2InventionJobs(const Names& names,
                                             Stuff& stuff,
                                             const double inventionTax,
                                             const JobValues& values,
                                             const double researchTimeReduction) const;
    std::vector<Job::Ptr> getT3InventionJobs(const Names& names,
                                             Stuff& stuff,
                                             const double inventionTax,
                                             const JobValues& values,
                                             const double researchTimeReduction) const;

    void filter(const Skills& skills);

  private:
    research_t t2Blueprints_;
    research_t t3Blueprints_;
};
