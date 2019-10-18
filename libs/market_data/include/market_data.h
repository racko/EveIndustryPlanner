#pragma once

#include "job.h"
#include "order.h"
#include "sqlite.h"
#include <iosfwd>
#include <string>
#include <unordered_map>
#include <vector>

struct Names;
struct Stuff;
struct Types;

struct Prices {
    void loadPrices(Names& names);

    std::unordered_map<std::size_t, double> avgPrices;
    std::unordered_map<std::size_t, double> adjPrices;
};

struct Orders {
    void loadOrdersFromDB(const sqlite::dbPtr& db,
                          const std::string& buyQuery,
                          const std::string& sellQuery,
                          const Names& names);

    std::vector<Job::Ptr>
    getMarketJobs(const Names& names, Stuff& stuff, const Types& types, double brokersFee, double salesTax) const;

    void serialize(std::ostream&) const;
    void deserialize(std::istream&);

    std::vector<Order> sellorders;
    std::vector<Order> buyorders;
};

struct MarketData : private Prices, private Orders {
    using Prices::adjPrices;
    using Prices::avgPrices;
    using Prices::loadPrices;

    using Orders::buyorders;
    using Orders::deserialize;
    using Orders::getMarketJobs;
    using Orders::loadOrdersFromDB;
    using Orders::sellorders;
    using Orders::serialize;
};
