#pragma once

#include "blueprint_selection.h"
#include "facility_costs.h"
#include "industry_limits.h"
#include "queries.h"
#include "skill_settings.h"
#include "trade_costs.h"

struct Settings {
    SkillSettings skills;
    TradeCosts trade;
    FacilityCosts facilities;
    Queries queries;
    IndustryLimits industry;
    BluePrintSelection blueprints;

    void read();
};
