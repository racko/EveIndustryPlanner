#include "planner.h"

#include "buy.h"
#include "sell.h"

#include <map>
#include <unordered_map>

// visit
// spanning hypertree
// flow
// potential
// primal <- flow
// dual <- potential

void Planner::addJob(const Job::Ptr& job) {
    if (colNameSet.has(job->getName().c_str()))
        throw std::runtime_error("Already have job '" + job->getName() + "'");

    const auto& out = job->getOutputs();
    const auto& in = job->getInputs();

    // std::cout << "Adding job " << job->getName() << ", id " << n << ", value " << job->getCost() << ", " <<
    // job->getLowerLimit() << " <= vol <= " << job->getUpperLimit() << '\n';
    colNameSet.add(job->getName().c_str());

    jobs.push_back(job);
    // std::cout << "colNameSet.size(): " << colNameSet.size() << '\n';

    soplex::DSVector col(int(out.size() + in.size() + (job->getCost() < 0)));

    if (job->getCost() < 0) {
        // std::cout << "  job has negative value. Added to buyInRow\n";
        // buyInRow.add(n, -job->getCost());
        col.add(0, -job->getCost()); // buy-in row
    }

    // std::cout << "  outputs:\n";
    for (const auto& o : out) {
        // std::cout << "    " << o.second << " " << o.first->getName() << '\n';
        assertResource(o.first);
        // constraints.at(o.first->rowID).add(n, -o.second);
        col.add(int(o.first->rowID), -o.second);
    }
    // std::cout << "  inputs:\n";
    for (const auto& i : in) {
        // std::cout << "    " << i.second << " " << i.first->getName() << '\n';
        assertResource(i.first);
        // constraints.at(i.first->rowID).add(n, i.second);
        col.add(int(i.first->rowID), i.second);
    }

    soplex::DataKey key;
    cols.add(key, job->getCost(), job->getLowerLimit(), col, job->getUpperLimit());
    job->colID = key.getIdx();

    if (cols.num() != colNameSet.size()) {
        std::cout << std::flush;
        throw std::runtime_error("column counts inconsistent");
    }

    // if (!cols.isConsistent())
    //    throw std::logic_error("cols is not consistent");
}

soplex::SoPlex& Planner::getLP(double buyIn) {
    if (!cols.isConsistent())
        throw std::logic_error("cols is not consistent");

    lp.addColsReal(cols);

    // std::cout << "Adding row " << 0 << " (BuyIn)\n";

    // lp.addRowReal(LPRow(buyInRow, LPRow::LESS_EQUAL, buyIn));
    // lp.addRowReal(LPRow(dummy, LPRow::LESS_EQUAL, buyIn));
    lp.changeRangeReal(0, -infinity, buyIn);
    rowNameSet.add("BuyIn");

    // std::cout << "lp.numRowsReal(): " << lp.numRowsReal() << '\n';
    // std::cout << "rowNameSet.size(): " << rowNameSet.size() << '\n';
    for (auto i = 0u; i < resources.size(); ++i) {
        // const auto& constraint = constraints[i];
        const auto& resource = resources[i];
        // if (constraint.size() > 0) {
        // std::cout << "Adding row " << (i+1) << " (" << resource->getName() << ")\n";
        rowNameSet.add(resource->getName().c_str());
        // - upper limit is always infinity
        // - We choose row >= lower when lower != 0
        // - otherwise row = 0
        // - Equations make the optimization problem easier, so why not do row = lower when lower != 0?
        //   We have a number of these lower bounds that conflict with one another. E.g. Given the limit on production lines or science labs,
        //   we cannot spend all our money. We could add "waste" actions with zero cost/value to turn these into equalities, but that's probably
        //   exactly what the solver is doing internally (add slack variables)
        lp.changeRangeReal(int(i + 1),
                           -resource->getLowerLimit() == 0 ? -resource->getLowerLimit() : -resource->getUpperLimit(),
                           -resource->getLowerLimit());

        // auto row = LPRow(-resource->getUpperLimit(), constraint, -resource->getLowerLimit());
        // if (!row.isConsistent())
        //    throw std::logic_error("row " + resource->getName() + " is not consistent");
        // lp.addRowReal(std::move(row));
        // std::cout << "lp.numRowsReal(): " << lp.numRowsReal() << '\n';
        // std::cout << "rowNameSet.size(): " << rowNameSet.size() << '\n';
        // if (lp.numRowsReal() != rowNameSet.size()) {
        //    std::cout << std::flush;
        //    throw std::runtime_error("row counts inconsistent");
        //}
        //}
    }
    // soplex::DSVector bla;
    // bla.add(6856, 1.0);
    // lp.addRowReal(LPRow(bla, LPRow::GREATER_EQUAL, 1));

    // lp.writeFileReal("/tmp/eve.lp", &rowNameSet, &colNameSet, nullptr);
    std::cout << std::flush;
    // lp.setIntParam(SoPlex::VERBOSITY, SoPlex::VERBOSITY_FULL);
    auto stat = lp.solve();
    soplex::DVector prim(lp.numColsReal());
    soplex::DVector dual(lp.numRowsReal());

    /* get solution */
    if (stat == soplex::SPxSolver::OPTIMAL) {
        lp.getPrimalReal(prim);
        lp.getDualReal(dual);
        std::cout << "Dual solution is \n";
        if (std::fabs(dual[0]) > 1e-16) {
            std::cout << "BuyIn: " << dual[0] << '\n';
        }
        for (auto i = 1; i < dual.dim(); ++i) {
            if (std::fabs(dual[i]) > 1e-16) {
                std::cout << resources.at(std::size_t(i) - 1u)->getFullName() << ": " << dual[i] << '\n';
            }
        }
        // std::cout << "extra constraint: " << dual[dual.dim()-1] << '\n';
        std::cout << "Primal solution is \n";
        for (auto i = 0; i < prim.dim(); ++i) {
            if (std::fabs(prim[i]) > 1e-16) {
                std::cout << jobs.at(std::size_t(i))->getFullName() << "," << prim[i] << "," << lp.objReal(i) << '\n';
            }
        }
        std::multimap<std::size_t, std::pair<std::size_t, std::string>> shoppinglist;
        std::multimap<std::size_t, std::pair<std::size_t, std::string>> sell_list;
        std::unordered_map<std::size_t, std::string> stations;
        stations.reserve(100);
        for (auto i = 0; i < prim.dim(); ++i) {
            if (std::fabs(prim[i]) > 1e-16) {
                if (const auto& buy_job = dynamic_cast<const Buy*>(jobs.at(std::size_t(i)).get())) {
                    auto station = buy_job->stationID;
                    auto count = std::llround(prim[i]);

                    const auto& name = buy_job->resource->getFullName();
                    shoppinglist.emplace(station, std::make_pair(count, name));
                    stations.emplace(station, buy_job->stationName);
                } else if (const auto& sell_job = dynamic_cast<const Sell*>(jobs.at(std::size_t(i)).get())) {
                    auto station = sell_job->stationID;
                    auto count = std::llround(prim[i]);

                    const auto& name = sell_job->resource->getFullName();
                    sell_list.emplace(station, std::make_pair(count, name));
                    stations.emplace(station, sell_job->stationName);
                }
            }
        }

        std::cout << "Shopping list:\n";
        for (const auto& station : stations) {
            std::cout << "\n" << station.second << ":\n";
            auto x = shoppinglist.equal_range(station.first);
            for (auto i = x.first; i != x.second; ++i)
                std::cout << i->second.first << " " << i->second.second << '\n';
        }
        std::cout << "Sell list:\n";
        for (const auto& station : stations) {
            std::cout << "\n" << station.second << ":\n";
            auto x = sell_list.equal_range(station.first);
            for (auto i = x.first; i != x.second; ++i)
                std::cout << i->second.first << " " << i->second.second << '\n';
        }
        std::cout << std::flush;

        // std::ofstream graph("/tmp/plan.dot");
        // graph << "digraph G {\n";
        // for (auto i = 0u; i < prim.dim(); ++i) {
        //    if (std::fabs(prim[i]) > 1e-16) {
        //        const auto& job = jobs.at(i);
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
        //        const auto& job = jobs.at(i);
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
        //        const auto& job = jobs.at(i);

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
        //    if (const auto& job = dynamic_cast<const Sell*>(jobs.at(ix).get())) {
        //        //auto station = job->stationID;
        //        //auto count = std::llround(prim[ix]);
        //
        //        const auto& name = job->resource->getFullName();
        //        std::cout << name << ": " << v.value;
        //    }
        //}

        std::cout << "LP solved to optimality.\n";
        std::cout << "Objective value is " << lp.objValueReal() << ".\n";
    } else {
        std::cout << "something failed\n";
    }
    return lp;
}

void Planner::assertResource(const Resource_Base::Ptr& resource) {
    if (std::find(resources.begin(), resources.end(), resource) != resources.end())
        return;
    auto m = resources.size() + 1; // TODO: Make buy-in a resource
    resources.push_back(resource);
    resource->rowID = m;
    // constraints.emplace_back();
    // std::cout << "      New resource! ID: " << m << '\n';
}
