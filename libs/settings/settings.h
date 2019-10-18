#pragma once

#include "libs/settings/blueprint_selection.h"
#include "libs/settings/facility_costs.h"
#include "libs/settings/industry_limits.h"
#include "libs/settings/queries.h"
#include "libs/settings/skill_settings.h"
#include "libs/settings/trade_costs.h"

struct Settings {
    SkillSettings skills;
    TradeCosts trade;
    FacilityCosts facilities;
    Queries queries;
    IndustryLimits industry;
    BluePrintSelection blueprints;

    void read();
};
