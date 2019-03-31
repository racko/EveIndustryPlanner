#pragma once

#include "settings.h"
#include "skills.h"
#include "assets.h"
#include "market_data.h"
#include "job_values.h"
#include "names.h"
#include "groups.h"
#include "jobs.h"
#include "types.h"
#include "planner.h"

struct Stuff;

struct workspace {
    workspace();

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

    Planner planner;

    void processBlueprints();

    void feedPlanner();

    void addPlanetary(Stuff& stuff);
    void addMarket(Stuff& stuff);
    void addProduction(Stuff& stuff);
    void addT2Invention(Stuff& stuff);
    void addT3Invention(Stuff& stuff);
    void addCopy(Stuff& stuff);
};
