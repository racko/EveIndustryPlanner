#pragma once

#include "resource_base.h"
#include <infinity.h>
#include <memory>
#include <string>
#include <vector>

class Job {
  public:
    using Ptr = std::shared_ptr<Job>;
    using ConstPtr = std::shared_ptr<const Job>;
    using Resources = std::vector<std::pair<Resource_Base::Ptr, double>>;

    Job(Resources i, Resources o, double c, std::string n, std::string fn, double l = 0, double u = infinity)
        : inputs(std::move(i)), outputs(std::move(o)), cost(c), lower_limit(l), upper_limit(u), name(std::move(n)),
          fullName(std::move(fn)) {}
    virtual ~Job() = default;
    const Resources& getInputs() const {
        return inputs;
        // return const_cast<Job*>(this)->getInputs();
    }
    const Resources& getOutputs() const {
        return outputs;
        // return const_cast<Job*>(this)->getOutputs();
    }
    Resources& getInputs() { return inputs; }
    Resources& getOutputs() { return outputs; }
    double getCost() const { return cost; }
    double getLowerLimit() const { return lower_limit; }
    double getUpperLimit() const { return upper_limit; }
    const std::string& getName() const { return name; }
    const std::string& getFullName() const { return fullName; }
    int colID;

  private:
    Resources inputs;
    Resources outputs;
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
