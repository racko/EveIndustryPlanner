#include "market_data.h"

#include "buy.h"
#include "names.h"
#include "sell.h"
#include "stuff.h"
#include "types.h"
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <fstream>
#include <iostream>
#include <json/json.h>
#include <order_serialization.h>
#include <profiling.h>

namespace {
Json::Value getRoot() {
    Json::Value root;
    std::ifstream pricesFile("prices.json", std::ifstream::binary);
    std::cout << "loading prices.json. " << std::flush;
    auto pTime = time<float, std::milli>([&] { pricesFile >> root; });
    std::cout << pTime.count() << "ms\n";
    return root;
}

Json::Value getItems() {
    auto items = getRoot();
    std::cout << "Have " << items.size() << " prices\n";
    return items;
}
} // namespace

void Prices::loadPrices(Names& names) {
    auto items = getItems();
    avgPrices.reserve(items.size());
    std::cout << "building map. " << std::flush;
    auto pTime2 = time<float, std::milli>([&] {
        for (const auto& e : items) {
            auto id = e["type"]["id"].asUInt64();
            const auto& averagePrice = e["averagePrice"];
            if (!averagePrice.isNull())
                avgPrices[id] = averagePrice.asDouble();
            const auto& adjustedPrice = e["adjustedPrice"];
            if (!adjustedPrice.isNull())
                adjPrices[id] = adjustedPrice.asDouble();
            names.ensureName(id, e["type"]["name"].asString());
        }
    });
    std::cout << pTime2.count() << "ms\n";
}

// void MarketData::loadHistoriesFromDB(const sqlite::dbPtr& db) {
//    //std::unordered_map<TypeID, double> latestPrices;
//    //latestPrices.reserve(priceCount);
//    //std::unordered_map<TypeID, double> volumes;
//    //volumes.reserve(priceCount);
//    auto selectBuy = sqlite::prepare(db, buyQuery);
//    //auto avgVolumeQuery = sqlite::prepare(db, volumeQuery);
//    buy.reserve(priceCount);
//    while(sqlite::step(selectBuy) == sqlite::ROW) {
//        auto id = sqlite::column<std::int64_t>(selectBuy, 0);
//        buy[id] = sqlite::column<double>(selectBuy, 1);
//        //latestPrices[id] = avg;
//        //avgPrices[id] = avg;
//    }
//    auto selectSell = sqlite::prepare(db, sellQuery);
//    sell.reserve(priceCount);
//    while(sqlite::step(selectSell) == sqlite::ROW) {
//        auto id = sqlite::column<std::int64_t>(selectSell, 0);
//        sell[id] = sqlite::column<double>(selectSell, 1);
//        //latestPrices[id] = avg;
//        //avgPrices[id] = avg;
//    }
//    //while(sqlite::step(avgVolumeQuery) == sqlite::ROW) {
//    //    auto id = sqlite::column<std::int64_t>(avgVolumeQuery, 0);
//    //    auto avg = sqlite::column<double>(avgVolumeQuery, 1);
//    //    volumes[id] = avg;
//    //}
//}

void Orders::loadOrdersFromDB(const sqlite::dbPtr& db,
                              const std::string& buyQuery,
                              const std::string& sellQuery,
                              const Names& names) {
    auto selectBuy = sqlite::prepare(db, buyQuery);
    buyorders.reserve(100000);
    while (sqlite::step(selectBuy) == sqlite::ROW) {
        auto type = sqlite::column<std::int64_t>(selectBuy, 1);
        if (!names.checkName(std::size_t(type))) {
            std::cout << "ignoring typeID " << type << ": no name\n";
            continue;
        }
        auto id = sqlite::column<std::int64_t>(selectBuy, 0);
        auto volume = sqlite::column<std::int64_t>(selectBuy, 2);
        auto price = sqlite::column<double>(selectBuy, 3);
        auto stationID = sqlite::column<std::int64_t>(selectBuy, 4);
        const auto& stationName = sqlite::column<std::string>(selectBuy, 5);
        buyorders.emplace_back(id, type, price, volume, stationID, stationName);
    }
    auto selectSell = sqlite::prepare(db, sellQuery);
    sellorders.reserve(200000);
    while (sqlite::step(selectSell) == sqlite::ROW) {
        auto type = sqlite::column<std::int64_t>(selectSell, 1);
        if (!names.checkName(std::size_t(type))) {
            std::cout << "ignoring typeID " << type << ": no name\n";
            continue;
        }
        auto id = sqlite::column<std::int64_t>(selectSell, 0);
        auto volume = sqlite::column<std::int64_t>(selectSell, 2);
        auto price = sqlite::column<double>(selectSell, 3);
        auto stationID = sqlite::column<std::int64_t>(selectSell, 4);
        const auto& stationName = sqlite::column<std::string>(selectSell, 5);
        sellorders.emplace_back(id, type, price, volume, stationID, stationName);
    }
}

std::vector<Job::Ptr> Orders::getMarketJobs([[maybe_unused]] const Names& names,
                                            Stuff& stuff,
                                            const Types& types,
                                            const double brokersFee,
                                            const double salesTax) const {
    std::vector<Job::Ptr> jobs;
    jobs.reserve(sellorders.size() + buyorders.size());
    std::cout << "  adding sell jobs\n";
    for (const auto& s : sellorders) {
        assert(names.checkName(s.type));
        if (types.isBP(s.type))
            continue;
        jobs.push_back(std::make_unique<Buy>(stuff.getResource(s.type),
                                             s.id,
                                             -s.price * (1.0 + brokersFee),
                                             double(s.volume),
                                             s.stationID,
                                             s.stationName));
    }
    std::cout << "  adding buy jobs\n";
    for (const auto& b : buyorders) {
        assert(names.checkName(b.type));
        if (types.isBP(b.type))
            continue;
        jobs.push_back(std::make_unique<Sell>(stuff.getResource(b.type),
                                              b.id,
                                              b.price * (1.0 - brokersFee - salesTax),
                                              double(b.volume),
                                              b.stationID,
                                              b.stationName));
    }
    return jobs;
}

void Orders::deserialize(std::istream& s) {
    boost::archive::binary_iarchive iarch(s);
    iarch >> sellorders;
    iarch >> buyorders;
}

void Orders::serialize(std::ostream& s) const {
    boost::archive::binary_oarchive oarch(s);
    oarch << sellorders;
    oarch << buyorders;
}

// void loadOrdersFromDB(const sqlite::dbPtr& db) {
//    //std::unordered_map<TypeID, double> latestPrices;
//    //latestPrices.reserve(priceCount);
//    //std::unordered_map<TypeID, double> volumes;
//    //volumes.reserve(priceCount);
//    auto selectBuy = sqlite::prepare(db, buyQuery);
//    auto avgVolumeQuery = sqlite::prepare(db, volumeQuery);
//    buy.reserve(priceCount);
//    while(sqlite::step(selectBuy) == sqlite::ROW) {
//        auto id = sqlite::column<std::int64_t>(selectBuy, 0);
//        buy[id] = sqlite::column<double>(selectBuy, 1);
//        //latestPrices[id] = avg;
//        //avgPrices[id] = avg;
//    }
//    auto selectSell = sqlite::prepare(db, sellQuery);
//    sell.reserve(priceCount);
//    while(sqlite::step(selectSell) == sqlite::ROW) {
//        auto id = sqlite::column<std::int64_t>(selectSell, 0);
//        sell[id] = sqlite::column<double>(selectSell, 1);
//        //latestPrices[id] = avg;
//        //avgPrices[id] = avg;
//    }
//    while(sqlite::step(avgVolumeQuery) == sqlite::ROW) {
//        auto id = sqlite::column<std::int64_t>(avgVolumeQuery, 0);
//        auto avg = sqlite::column<double>(avgVolumeQuery, 1);
//        volumes[id] = avg;
//    }
//}

// void loadHistories() {
//    std::cout << "Volumes:\n";
//    auto now = std::chrono::system_clock::now();
//    auto i = 0u;
//    auto count = avgPrices.size();
//    std::stringstream datestr;
//    for (auto& p : avgPrices) {
//        Json::Value root;
//        std::ifstream pricesFile(std::string("/tmp/histories/") + std::to_string(p.first) + ".json",
//        std::ifstream::binary); auto pTime = time<float,std::milli>([&] { pricesFile >> root; }); const auto& items =
//        root["items"]; std::cout << names.getName(p.first) << ":\n"; if (items.isNull()) {
//            volumes.insert({p.first, 0}); // no history means no sales volume (we separated production and sell, so
//            this does not mean that we cannot produce this item) std::cout << "  no volume\n"; continue;
//        }
//        double avg = 0;
//        double avgOrderCount = 0;
//        double avgPrice = 0;
//        double lowPrice = infinity;
//        auto count3 = 0u;
//        auto count30 = 0u;
//        double latestPrice = 0;
//        auto latest = std::chrono::system_clock::time_point::min();
//        for (const auto& item : items) {
//            datestr << item["date"].asString();
//            //std::cout << datestr.str();
//            std::tm time = {};
//            datestr >> std::get_time(&time, "%Y-%m-%d");
//            if (!datestr)
//                throw std::runtime_error("parse failed");
//            //std::cout << " " << (1900 + time.tm_year) << "-" << (time.tm_mon + 1) << "-" << time.tm_mday;
//            auto volume = item["volume"].asDouble();
//            //std::cout << " " << volume;
//            datestr.seekg(0);
//            datestr.seekp(0);
//            auto then_time_t = std::mktime(&time);
//            auto then = std::chrono::system_clock::from_time_t(then_time_t);
//            //std::cout << " " << then_time_t << " " << then.time_since_epoch().count();
//            auto diff = now - then;
//            //std::cout << " " << diff.count() << " (" <<
//            (double(std::chrono::duration_cast<std::chrono::seconds>(diff).count()) / 86400.0) << " days)"; auto
//            orderCount = item["orderCount"].asUInt64(); if (diff < std::chrono::seconds(86400 * 30)) {
//                ++count30;
//                avgOrderCount += (double(orderCount) - avgOrderCount) / double(count30);
//                avg += (volume - avg) / double(count30); // maximum number of days (found for tritanium etc.)
//                //std::cout << " ACCEPTED";
//            }

//            auto currentPrice = item["avgPrice"].asDouble();
//            auto currentLowPrice = item["lowPrice"].asDouble();
//            if (diff < std::chrono::seconds(86400 * 8)) { // week: 8
//                ++count3;
//                avgPrice += (currentPrice - avgPrice) / double(count3);
//                lowPrice = std::min(lowPrice, currentLowPrice);
//            }
//            if (then > latest) {
//                latest = then;
//                latestPrice = currentPrice;
//            }
//            //std::cout << '\n';
//        }
//        lowPrices[p.first] = lowPrice;
//        //p.second = avgPrice;
//        p.second = latestPrice;
//        std::cout << "  average Volume: " << avg << " (over " << count30 << " recorded days of trading)\n";
//        std::cout << "  latest Price: " << latestPrice << " (" << ((now - latest).count() / 1e9 / 3600.0 / 24.0) << "
//        days old)\n"; std::cout << "  average Price: " << avgPrice << " (over " << count3 << " recorded days of
//        trading)\n"; if (avgOrderCount < 100)
//            avg = 0;
//        volumes.insert({p.first, avg});
//        static constexpr const char* ANSI_CLEAR_LINE = "\033[2K";
//        ++i;
//        //std::cout << ANSI_CLEAR_LINE << " " << i << " / " << count << " | " << (double(i) / double(count) * 100.0)
//        << "%\r" << std::flush;
//    }
//}

// void setBuySell() {
//    sell.reserve(priceCount);
//    buy.reserve(priceCount);
//    for (const auto& p : avgPrices) {
//        //auto low = lowPrices.find(p.first);
//        //if (low != lowPrices.end())
//        //    std::tie(sell[p.first], buy[p.first]) = std::minmax(p.second, low->second);
//        //else
//            sell[p.first] = buy[p.first] = p.second;
//    }
//}
