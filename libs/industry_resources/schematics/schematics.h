#pragma once

#include "libs/industry_resources/job.h"
#include "libs/industry_resources/planetary.h"
#include <vector>

struct Stuff;
struct Assets;
struct Names;

struct Schematics {
    using schematics_t = std::vector<Planetary>;

    void loadSchematics();

    std::vector<Job::Ptr> getPlanetaryJobs(
        Stuff& stuff, const Assets& assets, const Names& names, double cpu, double power, double timeFrame) const;

    const schematics_t& getSchematics() const& { return schematics_; }
    void reserve(std::size_t n);

  private:
    schematics_t schematics_;
    std::size_t typeCount;
};
