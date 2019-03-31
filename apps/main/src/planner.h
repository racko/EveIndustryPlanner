#pragma once

#include "job.h"
#include "resource_base.h"

#include "soplex.h"
#include <vector>

//using namespace soplex;

class Planner {
public:
    Planner() : cols(1000000, 1000000) {}
    void addJob(const Job::Ptr& job);
    soplex::SoPlex& getLP(double buyIn);
private:
    void assertResource(const Resource_Base::Ptr& resource);

    soplex::SoPlex lp;
    soplex::NameSet colNameSet, rowNameSet;
    std::vector<Resource_Base::ConstPtr> resources;
    std::vector<Job::ConstPtr> jobs;
    //std::vector<DSVector> constraints;
    soplex::LPColSetReal cols;
    //DSVector buyInRow;
};

//class EveIndustryPlanner : public Planner {
//public:
//    EveIndustryPlanner(double buyIn) : Planner(buyIn), productionLineTime(std::make_shared<Resource_Base>("ProductionLineTime")), scienceLabTime(std::make_shared<Resource_Base>("ScienceLabTime")) {}
//    Resource_Base::Ptr getProductionLineTime() {
//        return productionLineTime;
//    }
//    Resource_Base::Ptr getScienceLabTime() {
//        return scienceLabTime;
//    }
//private:
//    Resource_Base::Ptr productionLineTime;
//    Resource_Base::Ptr scienceLabTime;
//};
