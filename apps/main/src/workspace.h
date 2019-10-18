#pragma once

#include "jobs.h"
#include "libs/assets/assets.h"
#include "libs/groups/groups.h"
#include "libs/industry_resources/job_values.h"
#include "libs/industry_resources/types/types.h"
#include "libs/market_data/market_data.h"
#include "libs/names/names.h"
#include "libs/settings/settings.h"
#include "skills.h"

class workspace {
  public:
    workspace();

    const Settings& getSettings() const { return settings_; }
    const Skills& getSkills() const { return skills_; }
    const Assets& getAssets() const { return assets_; }
    const MarketData& getMarketData() const { return market_; }
    const JobValues& getJobValues() const { return values_; }
    const Names& getNames() const { return names_; }
    const Groups& getGroups() const { return groups_; }
    const Jobs& getJobs() const { return jobs_; }
    const Types& getTypes() const { return types_; }

  private:
    Settings settings_;
    Skills skills_;
    Assets assets_;
    MarketData market_;
    JobValues values_;
    Names names_;
    Groups groups_;
    Jobs jobs_;
    Types types_;

    void processBlueprints();
    void feedPlanner();
    void loadTypeIds();
    void loadMarketData();
};
