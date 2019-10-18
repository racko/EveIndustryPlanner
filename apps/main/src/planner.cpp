#include "planner.h"

#include "buy.h"
#include "sell.h"
#include <infinity.h>
#include <map>
#include <unordered_map>

// visit
// spanning hypertree
// flow
// potential
// primal <- flow
// dual <- potential

Planner::Planner() : cols_{1000000, 1000000}, buyIn_{AddBuyIn()} {}
soplex::DataKey Planner::AddBuyIn() {
    soplex::DataKey key;
    rowNameSet_.add(key, "BuyIn");
    return key;
}

Planner::~Planner() = default;

void Planner::addJob(Job::Ptr job) {
    if (colNameSet_.has(job->getName().c_str()))
        throw std::runtime_error("Already have job '" + job->getName() + "'");

    const auto& out = job->getOutputs();
    const auto& in = job->getInputs();

    // std::cout << "Adding job " << job->getName() << ", id " << n << ", value " << job->getCost() << ", " <<
    // job->getLowerLimit() << " <= vol <= " << job->getUpperLimit() << '\n';
    colNameSet_.add(job->getName().c_str());

    // std::cout << "colNameSet_.size(): " << colNameSet_.size() << '\n';

    soplex::DSVector col(int(out.size() + in.size() + (job->getCost() < 0)));

    if (job->getCost() < 0) {
        // std::cout << "  job has negative value. Added to buyInRow\n";
        // buyInRow.add(n, -job->getCost());
        col.add(0, -job->getCost()); // buy-in row
    }

    // std::cout << "  outputs:\n";
    for (const auto& o : out) {
        // std::cout << "    " << o.second << " " << o.first->getName() << '\n';
        ensureResource(o.first);
        // constraints.at(o.first->rowID).add(n, -o.second);
        col.add(o.first->rowID, -o.second);
    }
    // std::cout << "  inputs:\n";
    for (const auto& i : in) {
        // std::cout << "    " << i.second << " " << i.first->getName() << '\n';
        ensureResource(i.first);
        // constraints.at(i.first->rowID).add(n, i.second);
        col.add(i.first->rowID, i.second);
    }

    soplex::DataKey key;
    cols_.add(key, job->getCost(), job->getLowerLimit(), col, job->getUpperLimit());
    job->colID = key.getIdx();

    jobs_.push_back(std::move(job));

    if (cols_.num() != colNameSet_.size()) {
        throw std::runtime_error("column counts inconsistent");
    }

    // if (!cols_.isConsistent())
    //    throw std::logic_error("cols_ is not consistent");
}

void Planner::solve(const double buyIn) {
    if (!cols_.isConsistent())
        throw std::logic_error("cols_ is not consistent");

    soplex::SoPlex lp_;
    lp_.addColsReal(cols_);

    // std::cout << "Adding row " << 0 << " (BuyIn)\n";

    // TODO: check if I can do all the changeRangeReal as soon as I add the corresponding resource - before addColsReal

    // lp_.addRowReal(LPRow(buyInRow, LPRow::LESS_EQUAL, buyIn));
    // lp_.addRowReal(LPRow(dummy, LPRow::LESS_EQUAL, buyIn));
    lp_.changeRangeReal(buyIn_.getIdx(), -infinity, buyIn);

    // std::cout << "lp_.numRowsReal(): " << lp_.numRowsReal() << '\n';
    // std::cout << "rowNameSet_.size(): " << rowNameSet_.size() << '\n';
    for (const auto& resource : resources_) {
        // const auto& constraint = constraints[i];
        // const auto& resource = resources_[i];
        // if (constraint.size() > 0) {
        // - upper limit is always infinity
        // - We choose row >= lower when lower != 0
        // - otherwise row = 0
        // - Equations make the optimization problem easier, so why not do row = lower when lower != 0?
        //   We have a number of these lower bounds that conflict with one another. E.g. Given the limit on production
        //   lines or science labs, we cannot spend all our money. We could add "waste" actions with zero cost/value to
        //   turn these into equalities, but that's probably exactly what the solver is doing internally (add slack
        //   variables)
        // lp_.changeRangeReal(int(i + 1),
        //                   -resource->getLowerLimit() == 0 ? -resource->getLowerLimit() : -resource->getUpperLimit(),
        //                   -resource->getLowerLimit());
        lp_.changeRangeReal(resource->rowID,
                            -resource->getLowerLimit() == 0 ? -resource->getLowerLimit() : -resource->getUpperLimit(),
                            -resource->getLowerLimit());

        // auto row = LPRow(-resource->getUpperLimit(), constraint, -resource->getLowerLimit());
        // if (!row.isConsistent())
        //    throw std::logic_error("row " + resource->getName() + " is not consistent");
        // lp_.addRowReal(std::move(row));
        // std::cout << "lp_.numRowsReal(): " << lp_.numRowsReal() << '\n';
        // std::cout << "rowNameSet_.size(): " << rowNameSet_.size() << '\n';
        // if (lp_.numRowsReal() != rowNameSet_.size()) {
        //    std::cout << std::flush;
        //    throw std::runtime_error("row counts inconsistent");
        //}
        //}
    }
    // soplex::DSVector bla;
    // bla.add(6856, 1.0);
    // lp_.addRowReal(LPRow(bla, LPRow::GREATER_EQUAL, 1));

    // lp_.writeFileReal("/tmp/eve.lp", &rowNameSet_, &colNameSet_, nullptr);
    std::cout << std::flush;
    // lp_.setIntParam(SoPlex::VERBOSITY, SoPlex::VERBOSITY_FULL);

    /* get solution */
    if (lp_.solve() != soplex::SPxSolver::OPTIMAL) {
        std::cout << "something failed\n";
        return;
    }

    soplex::DVector prim(lp_.numColsReal());
    soplex::DVector dual(lp_.numRowsReal());

    lp_.getPrimalReal(prim);
    lp_.getDualReal(dual);
    std::cout << "Dual solution is \n";
    if (std::fabs(dual[0]) > 1e-16) {
        std::cout << "BuyIn: " << dual[0] << '\n';
    }
    for (const auto& resource : resources_) {
        const auto v = dual[resource->rowID];
        if (std::fabs(v) > 1e-16) {
            std::cout << resource->getFullName() << ": " << v << '\n';
        }
    }
    // std::cout << "extra constraint: " << dual[dual.dim()-1] << '\n';
    std::cout << "Primal solution is \n";
    for (const auto& job : jobs_) {
        const auto i = job->colID;
        const auto v = prim[i];
        if (std::fabs(v) > 1e-16) {
            std::cout << job->getFullName() << "," << v << "," << lp_.objReal(i) << '\n';
        }
    }
    std::multimap<std::size_t, std::pair<std::size_t, std::string>> shoppinglist;
    std::multimap<std::size_t, std::pair<std::size_t, std::string>> sell_list;
    std::unordered_map<std::size_t, std::string> stations;
    stations.reserve(100);
    for (const auto& job : jobs_) {
        const auto v = prim[job->colID];
        if (std::fabs(v) > 1e-16) {
            if (const auto& buy_job = dynamic_cast<const Buy*>(job.get())) {
                const auto station = buy_job->stationID;
                const auto count = std::llround(v);

                const auto& name = buy_job->resource->getFullName();
                shoppinglist.emplace(station, std::make_pair(count, name));
                stations.emplace(station, buy_job->stationName);
            } else if (const auto& sell_job = dynamic_cast<const Sell*>(job.get())) {
                const auto station = sell_job->stationID;
                const auto count = std::llround(v);

                const auto& name = sell_job->resource->getFullName();
                sell_list.emplace(station, std::make_pair(count, name));
                stations.emplace(station, sell_job->stationName);
            }
        }
    }

    std::cout << "Shopping list:\n";
    for (const auto& station : stations) {
        std::cout << "\n" << station.second << ":\n";
        const auto x = shoppinglist.equal_range(station.first);
        for (auto it = x.first; it != x.second; ++it)
            std::cout << it->second.first << " " << it->second.second << '\n';
    }
    std::cout << "Sell list:\n";
    for (const auto& station : stations) {
        std::cout << "\n" << station.second << ":\n";
        const auto x = sell_list.equal_range(station.first);
        for (auto it = x.first; it != x.second; ++it)
            std::cout << it->second.first << " " << it->second.second << '\n';
    }

    // std::ofstream graph("/tmp/plan.dot");
    // graph << "digraph G {\n";
    // for (auto i = 0u; i < prim.dim(); ++i) {
    //    if (std::fabs(prim[i]) > 1e-16) {
    //        const auto& job = jobs_.at(i);
    //        const auto& jobName = job->getFullName();
    //        const auto& inputs = job->getInputs();
    //        const auto& outputs = job->getOutputs();
    //        if (!inputs.empty()) {
    //            graph << "{";
    //            auto in = inputs.begin();
    //            graph << "\"" << in++->first->getFullName() << "\"";
    //            for (; in != inputs.end(); ++in) {
    //                graph << " \"" << in->first->getFullName() << "\"";
    //            }
    //            graph << "} -> ";
    //        }
    //        graph << "\"" << jobName << "\"";
    //        if (!outputs.empty()) {
    //            graph << " -> {";
    //            auto out = outputs.begin();
    //            graph << "\"" << out++->first->getFullName() << "\"";
    //            for (; out != outputs.end(); ++out) {
    //                graph << " \"" << out->first->getFullName() << "\"";
    //            }
    //            graph << "}";
    //        }
    //        graph << "\n";
    //    }
    //}
    // graph << "}\n";
    // graph.close();

    // auto profitsum = 0.0;
    // for (auto i = 0u; i < prim.dim(); ++i) {
    //    if (std::fabs(prim[i]) > 1e-16) {
    //        const auto& job = jobs_.at(i);
    //        const auto& jobName = job->getFullName();
    //        const auto& inputs = job->getInputs();
    //        const auto& outputs = job->getOutputs();
    //        auto insum = 0.0;
    //        for (const auto& in : inputs) {
    //            std::cout << "  " << in.first->getFullName() << ": " << in.second << " * " <<
    //            dual[in.first->getRowID() + 1] << '\n'; insum += in.second * dual[in.first->getRowID() + 1];
    //        }
    //        auto outsum = 0.0;
    //        for (const auto& out : outputs) {
    //            std::cout << "  " << out.first->getFullName() << ": " << out.second << " * " <<
    //            dual[out.first->getRowID() + 1] << '\n'; outsum += out.second * dual[out.first->getRowID() + 1];
    //        }
    //        auto profit = outsum - insum + job->getCost();
    //        std::cout << jobName << ": " << prim[i] << " * (" << outsum << " - " << insum << " + " <<
    //        job->getCost() << ") = " << (prim[i] * profit) << "\n"; profitsum += prim[i] * profit;
    //    }
    //}
    // std::cout << "total profit: " << profitsum << '\n';
    // std::cout << std::flush;

    // struct vertex {
    //    vertex(std::string n, double c, double initial_value) : name(std::move(n)), count(c), double
    //    value(initial_value) {} std::string name; double count; double value;
    //};
    // using graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, vertex, double,
    // boost::no_property>; using v_desc = boost::graph_traits<graph>::vertex_descriptor; graph g; for (auto i = 0u;
    // i < prim.dim(); ++i) {
    //    if (std::fabs(prim[i]) > 1e-16) {
    //        const auto& job = jobs_.at(i);

    //        boost::add_vertex({}, g);
    //    }
    //}

    // std::vector<v_desc> sorted;
    // sorted.reserve(boost::num_vertices(g));
    // topological_sort(g, std::back_inserter(sorted));

    // for (const auto& vd : sorted) {
    //    auto& v = g[vd];
    //    v.value = v.job->getCost();
    //    const auto& inputs = v.job->getInputs();
    //    auto labtime = std::find_if(inputs.begin(), inputs.end(), [] (Job::Resources::const_reference r) { return
    //    r.first->getFullName() == "LabTime"; }); auto linetime = std::find_if(inputs.begin(), inputs.end(), []
    //    (Job::Resources::const_reference r) { return r.first->getFullName() == "ProductionLineTime"; }); v.t =
    //    (labtime == inputs.end() ? 0.0 : labtime->second) + (linetime == inputs.end() ? 0.0 : linetime->second);
    //    //for (const auto& ) {
    //    //    const auto& w = g[*wd];
    //    //    v.value += w.value;
    //    //    v.t += w.t;
    //    //}
    //    auto ix = v.job->getColID();
    //    if (const auto& job = dynamic_cast<const Sell*>(jobs_.at(ix).get())) {
    //        //auto station = job->stationID;
    //        //auto count = std::llround(prim[ix]);
    //
    //        const auto& name = job->resource->getFullName();
    //        std::cout << name << ": " << v.value;
    //    }
    //}

    std::cout << "LP solved to optimality.\n";
    std::cout << "Objective value is " << lp_.objValueReal() << ".\n";
}

void Planner::ensureResource(const Resource_Base::Ptr& resource) {
    if (std::find(resources_.begin(), resources_.end(), resource) != resources_.end())
        return;
    soplex::DataKey key;
    rowNameSet_.add(key, resource->getName().c_str());
    resource->rowID = key.getIdx();
    resources_.push_back(resource);
    // constraints.emplace_back();
    // std::cout << "      New resource! ID: " << m << '\n';
}
