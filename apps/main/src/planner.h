#pragma once

#include "job.h"
#include "resource_base.h"

#include "soplex.h"
#include <vector>

// using namespace soplex;

class Planner {
  public:
    Planner();
    ~Planner();
    void addJob(Job::Ptr job);
    void solve(double buyIn);

  private:
    void ensureResource(const Resource_Base::Ptr& resource);
    soplex::DataKey AddBuyIn();

    soplex::NameSet colNameSet_, rowNameSet_;
    std::vector<Resource_Base::ConstPtr> resources_;
    std::vector<Job::ConstPtr> jobs_;
    // std::vector<DSVector> constraints;
    soplex::LPColSetReal cols_;
    soplex::DataKey buyIn_;
    // DSVector buyInRow;
};

// class EveIndustryPlanner : public Planner {
// public:
//    EveIndustryPlanner(double buyIn) : Planner(buyIn),
//    productionLineTime(std::make_shared<Resource_Base>("ProductionLineTime")),
//    scienceLabTime(std::make_shared<Resource_Base>("ScienceLabTime")) {} Resource_Base::Ptr getProductionLineTime() {
//        return productionLineTime;
//    }
//    Resource_Base::Ptr getScienceLabTime() {
//        return scienceLabTime;
//    }
// private:
//    Resource_Base::Ptr productionLineTime;
//    Resource_Base::Ptr scienceLabTime;
//};
