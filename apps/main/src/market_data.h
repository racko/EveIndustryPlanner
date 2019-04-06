#pragma once

#include "order.h"
#include "sqlite.h"

#include <string>
#include <unordered_map>
#include <vector>

struct Names;

struct MarketData {
    void loadPrices(Names& names);
    void loadOrdersFromDB(const sqlite::dbPtr& db,
                          const std::string& buyQuery,
                          const std::string& sellQuery,
                          const Names& names);
    // private:
    std::unordered_map<std::size_t, double> avgPrices;
    std::unordered_map<std::size_t, double> adjPrices;
    // std::unordered_map<std::size_t, double> lowPrices;
    // std::unordered_map<std::size_t, double> sell;
    // std::unordered_map<std::size_t, double> buy;
    std::vector<Order> sellorders;
    std::vector<Order> buyorders;
    std::size_t priceCount;
};
