#pragma once

#include "skill_settings.h"
#include "trade_costs.h"
#include "facility_costs.h"
#include "queries.h"
#include "industry_limits.h"
#include "blueprint_selection.h"

struct Settings {
    SkillSettings skills;
    TradeCosts trade;
    FacilityCosts facilities;
    Queries queries;
    IndustryLimits industry;
    BluePrintSelection blueprints;

    void read();
};
