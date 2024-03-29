#pragma once

#include "libs/industry_resources/resource_base.h"
#include <memory>
#include <string>
#include <vector>

class Job {
  public:
    using Ptr = std::unique_ptr<Job>;
    using ConstPtr = std::unique_ptr<const Job>;
    using MaterialRequirement = std::pair<Resource_Base::Ptr, double>;
    using Resources = std::vector<MaterialRequirement>;
    // helpers for argument readability
    using InputResources = Resources;
    using OutputResources = Resources;

    Job(Resources i, Resources o, double c, std::string n, std::string fn, double l, double u);
    Job(Resources i, Resources o, double c, std::string n, std::string fn, double l = 0);
    virtual ~Job();

    const InputResources& getInputs() const { return inputs; }
    const OutputResources& getOutputs() const { return outputs; }
    double getCost() const { return cost; }
    double getLowerLimit() const { return lower_limit; }
    double getUpperLimit() const { return upper_limit; }
    const std::string& getName() const { return name; }
    const std::string& getFullName() const { return fullName; }
    int colID;

  private:
    InputResources inputs;
    OutputResources outputs;
    double cost;
    double lower_limit;
    double upper_limit;
    std::string name;
    std::string fullName;
};

// class ProductionTimeSource : public Job {
// public:
//    ProductionTimeSource(double l) : Job({}, {{EveIndustryPlanner::getProductionLineTime(), 1}}, 0, "ProductionTime"),
//    limit(l) {}
// private:
//    double limit;
//};
//
// class LabTimeSource : public Job {
// public:
//    LabTimeSource(double l) : Job({}, {{EveIndustryPlanner::getScienceLabTime(), 1}}, 0), limit(l) {}
// private:
//    double limit;
//};
//
// class T1Production : public Job {
//    T1Production(Production p, Resources i, Resources o, double c) : Job(std::move(i), std::move(o), c),
//    production(std::move(p)) {}
// private:
//    Production production;
//};
//
// class Invention : public Job {
//    Invention(Research r, Resources i, Resources o, double c) : Job(std::move(i), std::move(o), c),
//    research(std::move(r)) {}
// private:
//    Research research;
//};
//
// class T2Production : public Job {
//    T2Production(Production p, Resources i, Resources o, double c) : Job(std::move(i), std::move(o), c),
//    production(std::move(p)) {}
// private:
//    Production production;
//};
