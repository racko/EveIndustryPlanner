#include "Finalizer.h"
#include "HTTPSRequestHandler.h"
#include "QueryRunner.h"
#include "Semaphore.h"
#include "outdatedOrders.h"
#include "profiling.h"
#include "sqlite.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <csignal>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <json/json.h>
#include <list>
#include <sstream>
#include <unordered_map>

static constexpr const char* ANSI_CLEAR_LINE = "\033[2K";

Semaphore handler_lock(1);
using handler_t = std::function<void()>;
using handlers_t = std::list<handler_t>;
handlers_t sigint_handlers; // list was chosen for iterator validity

auto addSigintHandler(std::function<void()> handler) {
    handler_lock.acquire();
    auto handle = sigint_handlers.insert(sigint_handlers.end(), handler);
    handler_lock.release();
    return handle;
}

void removeSigintHandler(handlers_t::const_iterator handle) { sigint_handlers.erase(handle); }

void sigint_handler(int) {
    for (auto h : sigint_handlers)
        h();
}

template <typename T>
auto openFile(const T& filename) {
    return std::ifstream(filename, std::ifstream::binary);
}

template <typename T>
auto parseStream(T& filestream) {
    Json::Value root;
    filestream >> root;
    return root;
}

template <typename T>
auto loadJSON(const T& filename) {
    auto filestream = openFile(filename);
    return parseStream(filestream);
}

auto loadOrders(const std::string& dir, std::size_t typeID) { return loadJSON(dir + std::to_string(typeID) + ".json"); }

template <typename Functor>
void for_each(const Json::Value& root, Functor f) {
    if (root.isNull())
        return;
    for (const auto& x : root)
        f(x);
}

template <typename Functor>
void for_each_price(const Json::Value& root, Functor f) {
    for_each(root, [&](const auto& x) { f(x["price"].asDouble()); });
}

template <typename T, typename Functor>
T accumulate(const Json::Value& root, T x, Functor f) {
    for_each(root["items"], [&](const auto& y) { x = f(x, y); });
    return x;
}

template <typename T, typename Functor>
T accumulate_price(const Json::Value& root, T x, Functor f) {
    for_each_price(root["items"], [&](double price) { x = f(x, price); });
    return x;
}

double getMaxPrice(const Json::Value& root) {
    // gcc 5.2.0 fails with "couldn't deduce template parameter ‘Functor’" if we try to pass &std::max<double> directly.
    // there is a cppcon (?) video where somebody explained that the standard does not guarantee that there is a
    // std::max<double> with the required signature. It only guarantees that std::max<double> can be called in some way
    // and will then return a double. But it could have default arguments or not be a function at all (it could be a
    // functor. They actually said "the 'correct' solution is to use a lambda."
    return accumulate_price(root, -std::numeric_limits<double>::infinity(),
                            [](double a, double b) { return std::max(a, b); });
}

double getMinPrice(const Json::Value& root) {
    return accumulate_price(root, std::numeric_limits<double>::infinity(),
                            [](double a, double b) { return std::min(a, b); });
}

double getMaxPrice(const std::string& dir, std::size_t typeID) { return getMaxPrice(loadOrders(dir, typeID)); }

double getMinPrice(const std::string& dir, std::size_t typeID) { return getMinPrice(loadOrders(dir, typeID)); }

double getMaxPriceInStation(const Json::Value& root, std::size_t stationId) {
    return accumulate(root, -std::numeric_limits<double>::infinity(), [&](double a, const Json::Value& v) {
        auto p1 = v["location"]["id"].asUInt64() == stationId;
        auto p2 = v["minVolume"].asUInt64() == 1;
        return p1 && p2 ? std::max(a, v["price"].asDouble()) : a;
    });
}

double getMinPriceInStation(const Json::Value& root, std::size_t stationId) {
    return accumulate(root, std::numeric_limits<double>::infinity(), [&](double a, const Json::Value& v) {
        return v["location"]["id"].asUInt64() == stationId ? std::min(a, v["price"].asDouble()) : a;
    });
}

double getMaxPriceInStation(const std::string& dir, std::size_t typeID, std::size_t stationId) {
    return getMaxPriceInStation(loadOrders(dir, typeID), stationId);
}

double getMinPriceInStation(const std::string& dir, std::size_t typeID, std::size_t stationId) {
    return getMinPriceInStation(loadOrders(dir, typeID), stationId);
}

void saveOrders(const std::unordered_map<std::size_t, double>& avgPrices, const sqlite::dbPtr& db) {
    auto insertRobustPrice = sqlite::prepare(db, "insert or replace into robust_price values(?, ?, ?);");
    auto i = 0u;
    auto count = avgPrices.size();
    auto inserted = 0u;
    for (auto& p : avgPrices) {
        auto incr_i = makeFinalizer([&] {
            ++i;
            std::cout << ANSI_CLEAR_LINE << " " << i << " / " << count << " | " << (double(i) / double(count) * 100.0)
                      << "%\r" << std::flush;
        });
        auto maxBuy = getMaxPriceInStation("/tmp/orders/buy/", p.first, 60003760);
        auto minSell = getMinPriceInStation("/tmp/orders/sell/", p.first, 60003760);
        // std::cout << p.first << ": " << "maxBuy = " << maxBuy << ", minSell = " << minSell << '\n';
        if (maxBuy < std::numeric_limits<double>::lowest() && minSell > std::numeric_limits<double>::max()) {
            continue;
        }
        sqlite::Resetter resetter(insertRobustPrice);
        sqlite::bind(insertRobustPrice, 1, p.first);
        sqlite::bind(insertRobustPrice, 2, minSell > std::numeric_limits<double>::max() ? maxBuy : minSell);   // buy
        sqlite::bind(insertRobustPrice, 3, maxBuy < std::numeric_limits<double>::lowest() ? minSell : maxBuy); // sell
        auto rc = sqlite::step(insertRobustPrice);
        if (rc != sqlite::DONE) {
            std::cout << __PRETTY_FUNCTION__ << ":  " << sqlite::errstr(rc) << " (" << rc << ")\n";
            continue;
        }
        ++inserted;
    }
    std::cout << "Done. Have prices for " << inserted << " items.\n";
}

namespace {
int64_t to_int(std::uint64_t n) {
    assert(n <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()));
    return static_cast<std::int64_t>(n);
}
} // namespace

class OrderHandler : public HTTPSRequestHandlerGroup {
  public:
    OrderHandler(std::size_t n, bool print_progress_)
        : HTTPSRequestHandlerGroup(n, "crest-tq.eveonline.com", 443), writer("market_history.db"),
          insertBuy_(
              writer.prepareStatement("insert or replace into robust_price(typeid, buy, sell, date) values(?, ?, "
                                      "(select sell from robust_price where typeid = ?), strftime('%s', 'now'));")),
          insertSell_(
              writer.prepareStatement("insert or replace into robust_price(typeid, buy, sell, date) values(?, (select "
                                      "buy from robust_price where typeid = ?), ?, strftime('%s', 'now'));")),
          print_progress(print_progress_) {}

    void saveOrders(const std::vector<std::size_t>& types /*, const std::string& access_token*/) {
        total = 2 * types.size();
        auto handler_handle = addSigintHandler([&] { semaphore.setTo(to_int(total)); });
        auto sigintHandlerRemover = makeFinalizer([&] { removeSigintHandler(handler_handle); });
        for (auto p : types) {
            buyTransactions.emplace_back(std::make_shared<BuyTransaction>(*this, p));
            postRequest(buyTransactions.back(), "");
            sellTransactions.emplace_back(std::make_shared<SellTransaction>(*this, p));
            postRequest(sellTransactions.back(), "");
        }
        semaphore.acquire(total);
    }

  private:
    auto releaseOne() {
        semaphore.release();
        return semaphore.get();
    }

    class BuyTransaction : public RequestWithResponseHandler, public Query {
      public:
        BuyTransaction(OrderHandler& parent_, std::size_t p_)
            : RequestWithResponseHandler("/market/10000002/orders/buy/?type=https://crest-tq.eveonline.com/types/" +
                                         std::to_string(p_) + "/"),
              parent(parent_), p(p_) {}
        void run(const sqlite::dbPtr&) override {
            auto& insertSell = parent.insertSell_;
            sqlite::Resetter resetter(
                insertSell); // handleBuy gets called for buy orders ... we want to SELL to buy orders
            sqlite::bind(insertSell, 1, p);
            sqlite::bind(insertSell, 2, p);
            if (maxBuy < std::numeric_limits<double>::lowest())
                sqlite::bind_null(insertSell, 3);
            else
                sqlite::bind(insertSell, 3, maxBuy);
            sqlite::step(insertSell);
            finalize();
        }

        void handleResponse(const Response&, std::istream& s) override {
            try {
                s >> responseBody.rdbuf();
                Json::Value orders;
                responseBody >> orders;
                maxBuy = getMaxPriceInStation(orders, 60003760);
            } catch (const std::exception& e) {
                onJsonError(e);
            }

            if (!std::isnan(maxBuy))
                parent.writer.withConnection(*this);
        }

        void onSqlError(const std::exception& e) override {
            std::cout << "Failed to update buy orders for item " << p << " with price " << maxBuy << ":\n"
                      << e.what() << '\n';
            finalize();
        }

        void onJsonError(const std::exception& e) {
            std::cout << "Failed to update buy orders for item " << p << ":\n" << e.what() << '\n';
            responseBody.seekg(0);
            std::cout << "response: \n" << responseBody.rdbuf() << '\n';
            finalize();
        }

        void onHttpError(const std::exception& e) override {
            std::cout << "Failed to update buy orders for item " << p << ":\n" << e.what() << '\n';
            finalize();
        }

        void finalize() {
            auto n = parent.releaseOne();
            std::cout << ANSI_CLEAR_LINE << " " << n << " / " << parent.total << " | "
                      << (double(n) / double(parent.total) * 100.0) << "%\r" << std::flush;
        }

      private:
        OrderHandler& parent;
        std::size_t p;
        double maxBuy = std::numeric_limits<double>::quiet_NaN();
        std::stringstream responseBody;
    };

    class SellTransaction : public RequestWithResponseHandler, public Query {
      public:
        SellTransaction(OrderHandler& parent_, std::size_t p_)
            : RequestWithResponseHandler("/market/10000002/orders/sell/?type=https://crest-tq.eveonline.com/types/" +
                                         std::to_string(p_) + "/"),
              parent(parent_), p(p_) {}
        void run(const sqlite::dbPtr&) override {
            auto& insertBuy = parent.insertBuy_;
            sqlite::Resetter resetter(insertBuy);
            sqlite::bind(insertBuy, 1, p);
            if (minSell > std::numeric_limits<double>::max())
                sqlite::bind_null(insertBuy, 2);
            else
                sqlite::bind(insertBuy, 2, minSell);
            sqlite::bind(insertBuy, 3, p);
            sqlite::step(insertBuy);
            finalize();
        }

        void handleResponse(const Response&, std::istream& s) override {
            try {
                s >> responseBody.rdbuf();
                Json::Value orders;
                responseBody >> orders;
                minSell = getMinPriceInStation(orders, 60003760);
            } catch (const std::exception& e) {
                onJsonError(e);
            }

            if (!std::isnan(minSell))
                parent.writer.withConnection(*this);
        }

        void onSqlError(const std::exception& e) override {
            std::cout << "Failed to update sell orders for item " << p << " with price " << minSell << ":\n"
                      << e.what() << '\n';
            finalize();
        }

        void onJsonError(const std::exception& e) {
            std::cout << "Failed to update sell orders for item " << p << ":\n" << e.what() << '\n';
            responseBody.seekg(0);
            std::cout << "response: \n" << responseBody.rdbuf() << '\n';
            finalize();
        }

        void onHttpError(const std::exception& e) override {
            std::cout << "Failed to update sell orders for item " << p << ":\n" << e.what() << '\n';
            finalize();
        }

        void finalize() {
            auto n = parent.releaseOne();
            if (parent.print_progress)
                std::cout << ANSI_CLEAR_LINE << " " << n << " / " << parent.total << " | "
                          << (double(n) / double(parent.total) * 100.0) << "%\r" << std::flush;
        }

      private:
        OrderHandler& parent;
        std::size_t p;
        double minSell = std::numeric_limits<double>::quiet_NaN();
        std::stringstream responseBody;
    };

    DatabaseConnection writer;
    sqlite::stmtPtr insertBuy_;
    sqlite::stmtPtr insertSell_;
    Semaphore semaphore;
    std::vector<std::shared_ptr<BuyTransaction>> buyTransactions;
    std::vector<std::shared_ptr<SellTransaction>> sellTransactions;
    std::size_t total;
    bool print_progress;
};

class OrderHandler2 {
  public:
    OrderHandler2(std::size_t r, std::size_t n, bool print_progress_)
        : region(r), writer("/home/racko/btsync/git/emdr/market_history.db"),
          insertStation_(
              writer.prepareStatement("insert or ignore into stations(stationid, name, regionid) values(?, ?, ?);")),
          insertOrder_(
              writer.prepareStatement("insert or replace into orders(orderid, typeid, station, volume_entered, "
                                      "duration, min_volume, active) values(?, ?, ?, ?, ?, ?, 1);")),
          insertBuyOrder_(writer.prepareStatement("insert or ignore into buy_orders(orderid, range) values(?, ?);")),
          insertSellOrder_(writer.prepareStatement("insert or ignore into sell_orders(orderid) values(?);")),
          insertOrderView_(writer.prepareStatement("insert into order_views(viewtime, orderid, price, volume, "
                                                   "start_date) values(strftime('%s', 'now'), ?, ?, ?, ?);")),
          http_handlers(n, "crest-tq.eveonline.com", 443), print_progress(print_progress_) {}

    void saveOrders(const std::vector<std::size_t>& types /*, const std::string& access_token*/) {
        total = types.size();
        auto handler_handle = addSigintHandler([&] { semaphore.setTo(to_int(total)); });
        auto sigintHandlerRemover = makeFinalizer([&] { removeSigintHandler(handler_handle); });
        // for (auto region : {10000002, 10000030, 10000032, 10000042, 10000043}) {
        for (auto p : types) {
            transactions.emplace_back(std::make_unique<Transaction>(*this, region, p));
            http_handlers.postRequest(*transactions.back(), "");
        }
        //}
        semaphore.acquire(total);
    }

  private:
    auto releaseOne() {
        semaphore.release();
        return semaphore.get();
    }

    class Transaction : public RequestWithResponseHandler {
      public:
        Transaction(OrderHandler2& parent_, std::size_t region, std::size_t p_)
            : RequestWithResponseHandler("/market/" + std::to_string(region) +
                                         "/orders/?type=https://crest-tq.eveonline.com/types/" + std::to_string(p_) +
                                         "/"),
              parent(parent_), p(p_), regionid(region) {}
        ~Transaction() { semaphore.acquire(singleOrderHandlers.size()); }

        void handleResponse(const Response&, std::istream& s) override {
            s >> responseBody.rdbuf();
            try {
                Json::Value root;
                responseBody >> root;
                orders = root["items"];
            } catch (const std::exception& e) {
                onJsonError(e);
                orders.clear();
            }
            singleOrderHandlers.reserve(orders.size());
            for (const auto& order : orders) {
                try {
                    singleOrderHandlers.emplace_back(*this, order, regionid);
                    parent.writer.withConnection(singleOrderHandlers.back());
                } catch (const std::exception& e) {
                    onJsonError(e, order);
                }
            }
            semaphore.waitFor([&](std::size_t n) { return n == singleOrderHandlers.size(); });
            finalize();
        }

        void onJsonError(const std::exception& e) {
            std::cout << "JSON error: Failed to update orders for item " << p << ":\n" << e.what() << '\n';
            responseBody.seekg(0);
            std::cout << "response: \n" << responseBody.rdbuf() << '\n';
        }

        void onJsonError(const std::exception& e, const Json::Value& order) {
            std::cout << "JSON error: Failed to update order for item " << p << ":\n" << e.what() << '\n';
            std::cout << "order: \n" << order << '\n';
        }

        void onHttpError(const std::exception& e) override {
            std::cout << "HTTP error: Failed to update orders for item " << p << ":\n" << e.what() << '\n';
            finalize();
        }

        void finalize() {
            auto n = parent.releaseOne();
            if (parent.print_progress)
                std::cout << ANSI_CLEAR_LINE << " " << n << " / " << parent.total << " | "
                          << (double(n) / double(parent.total) * 100.0) << "%\r" << std::flush;
        }

      private:
        class SingleOrderHandler : public Query {
          public:
            SingleOrderHandler(Transaction& parent_, const Json::Value& o, std::size_t region)
                : parent(parent_), order(o), regionid(region) {
                id = o["id"].asUInt64();
                type = o["type"]["id"].asUInt64();
                station = o["location"]["id"].asUInt64();
                station_name = o["location"]["name"].asString();
                volume_entered = o["volumeEntered"].asUInt64();
                std::tm time = {};
                auto result = strptime(o["issued"].asString().c_str(), "%Y-%m-%dT%T", &time);
                if (result == nullptr)
                    throw std::runtime_error("strptime failed on input '" + o["issued"].asString() + "'");
                start_date = timegm(&time);
                duration = o["duration"].asUInt64();
                range = o["range"].asString();
                price = o["price"].asDouble();
                volume = o["volume"].asUInt64();
                min_volume = o["minVolume"].asUInt64();
                buy = o["buy"].asBool();
            }

            void insStation() {
                auto& insertStation = parent.parent.insertStation_;
                sqlite::Resetter resetter(insertStation);
                sqlite::bind(insertStation, 1, station);
                sqlite::bind(insertStation, 2, station_name);
                sqlite::bind(insertStation, 3, regionid);
                sqlite::step(insertStation);
            }

            void insOrder() {
                // insertOrder(writer.prepareStatement("insert or ignore into orders(orderid, typeid, station,
                // volume_entered, duration, min_volume) values(?, ?, ?, ?, ?, ?);")),
                auto& insertOrder = parent.parent.insertOrder_;
                sqlite::Resetter resetter(insertOrder);
                sqlite::bind(insertOrder, 1, id);
                sqlite::bind(insertOrder, 2, type);
                sqlite::bind(insertOrder, 3, station);
                sqlite::bind(insertOrder, 4, volume_entered);
                sqlite::bind(insertOrder, 5, duration);
                sqlite::bind(insertOrder, 6, min_volume);
                sqlite::step(insertOrder);
            }

            void insBuyOrder() {
                // insertBuyOrder(writer.prepareStatement("insert or ignore into orders(orderid, range), values(?,
                // ?);")),
                auto& insertBuyOrder = parent.parent.insertBuyOrder_;
                sqlite::Resetter resetter(insertBuyOrder);
                sqlite::bind(insertBuyOrder, 1, id);
                sqlite::bind(insertBuyOrder, 2, range);
                sqlite::step(insertBuyOrder);
            }

            void insSellOrder() {
                // insertSellOrder(writer.prepareStatement("insert or ignore into sell_orders(orderid) values(?);")),
                auto& insertSellOrder = parent.parent.insertSellOrder_;
                sqlite::Resetter resetter(insertSellOrder);
                sqlite::bind(insertSellOrder, 1, id);
                sqlite::step(insertSellOrder);
            }

            void insOrderView() {
                // insertOrderView(writer.prepareStatement("insert into orders(viewtime, orderid, price, volume)
                // values(strftime('%s', 'now'), ?, ?, ?);")) {}
                auto& insertOrderView = parent.parent.insertOrderView_;
                sqlite::Resetter resetter(insertOrderView);
                sqlite::bind(insertOrderView, 1, id);
                sqlite::bind(insertOrderView, 2, price);
                sqlite::bind(insertOrderView, 3, volume);
                sqlite::bind(insertOrderView, 4, start_date);
                sqlite::step(insertOrderView);
            }

            void run(const sqlite::dbPtr&) override {
                insStation();
                insOrder();
                if (buy)
                    insBuyOrder();
                else
                    insSellOrder();
                insOrderView();
                finalize();
            }

            void onSqlError(const std::exception& e) override {
                std::cout << "SQL error: Failed to update order for item " << type << ":\n" << e.what() << '\n';
                std::cout << "order: \n" << order << '\n';
                finalize();
            }

            void finalize() { parent.semaphore.release(); }

          private:
            Transaction& parent;
            Json::Value order;
            std::size_t regionid;
            std::size_t id;
            std::size_t type;
            std::size_t station;
            std::string station_name;
            std::size_t volume_entered;
            time_t start_date;
            std::size_t duration;
            std::string range;
            double price;
            std::size_t volume;
            std::size_t min_volume;
            bool buy;
        };

        OrderHandler2& parent;
        std::size_t p;
        std::size_t regionid;

        std::stringstream responseBody;
        Json::Value orders;
        Semaphore semaphore;
        std::vector<SingleOrderHandler> singleOrderHandlers;
    };

    std::size_t region;
    DatabaseConnection writer;
    sqlite::stmtPtr insertStation_;
    sqlite::stmtPtr insertOrder_;
    sqlite::stmtPtr insertBuyOrder_;
    sqlite::stmtPtr insertSellOrder_;
    sqlite::stmtPtr insertOrderView_;
    Semaphore semaphore;
    std::vector<std::unique_ptr<Transaction>> transactions;
    HTTPSRequestHandlerGroup http_handlers;
    std::size_t total;
    bool print_progress;
};

class OrderHandler3 {
  public:
    OrderHandler3(std::size_t r, bool print_progress_)
        : region(r), writer("/home/racko/btsync/git/emdr/market_history.db"),
          insertOrder_(
              writer.prepareStatement("insert or replace into orders(orderid, typeid, station, volume_entered, "
                                      "duration, min_volume, active) values(?, ?, ?, ?, ?, ?, 1);")),
          insertBuyOrder_(writer.prepareStatement("insert or ignore into buy_orders(orderid, range) values(?, ?);")),
          insertSellOrder_(writer.prepareStatement("insert or ignore into sell_orders(orderid) values(?);")),
          insertOrderView_(writer.prepareStatement("insert or replace into order_views(viewtime, orderid, price, "
                                                   "volume, start_date) values(strftime('%s', 'now'), ?, ?, ?, ?);")),
          print_progress(print_progress_) {}

    ~OrderHandler3() { semaphore.acquire(singleOrderHandlers.size()); }

    auto releaseOne() {
        semaphore.release();
        return semaphore.get();
    }

    void finalize() {
        auto n = releaseOne();
        if (print_progress)
            std::cout << ANSI_CLEAR_LINE << " " << n << " / " << singleOrderHandlers.size() << " | "
                      << (double(n) / double(singleOrderHandlers.size()) * 100.0) << "%\r" << std::flush;
    }

    void onJsonError(const std::exception& e) {
        std::cout << "Failed to process stream:\n" << e.what() << '\n';
        stream.seekg(0);
        std::cout << "response: \n" << stream.rdbuf() << '\n';
        // finalize();
    }

    void onJsonError(const std::exception& e, const Json::Value& order) {
        std::cout << "JSON error: Failed to process order:\n" << e.what() << '\n';
        std::cout << "order: \n" << order << '\n';
    }

    void updateOrders() {
        // sqlite::Transaction transaction;
        auto interrupted = false;
        auto handler_handle = addSigintHandler([&] {
            interrupted = true; /*semaphore.setTo(std::numeric_limits<std::int32_t>::max());*/
        });
        auto sigintHandlerRemover = makeFinalizer([&] { removeSigintHandler(handler_handle); });
        TIME(simpleGet("https://esi.evetech.net/latest/markets/" + std::to_string(region) + "/orders/?page=1", stream););

        // auto rawtime = std::time(nullptr);
        // auto ptm = gmtime(&rawtime);

        // std::stringstream dirname;
        // dirname << std::setfill('0');
        // dirname << std::setw(4) << (1900 + ptm->tm_year);
        // dirname << "_" << std::setw(2) << (1 + ptm->tm_mon);
        // dirname << "_" << std::setw(2) << ptm->tm_mday;
        // dirname << "_" << std::setw(2) << ptm->tm_hour;
        // dirname << "_" << std::setw(2) << ptm->tm_min;
        // dirname << "_" << std::setw(2) << ptm->tm_sec;
        // namespace fs = boost::filesystem;
        // fs::path dir(fs::path("/tmp") / dirname.str());
        // fs::create_directories(dir);
        // std::ofstream f((dir / "1.json").native());
        // f << stream.rdbuf();
        // f.close();
        // stream.seekg(0);
        singleOrderHandlers.reserve(1000000); // reallocations cause undefined behaviour in sql threads
        std::size_t page = 1;
        try {
            do {
                Json::Value root;
                stream >> root;
                orders = root["items"];
                for (const auto& order : orders) {
                    try {
                        singleOrderHandlers.emplace_back(*this, order, region);
                        writer.withConnection(singleOrderHandlers.back());
                    } catch (const std::exception& e) {
                        onJsonError(e, order);
                    }
                }
                //////////////////////
                stream.str("");
                const auto& next = root["next"];
                if (!next.isNull()) {
                    ++page;
                    TIME(simpleGet(next["href"].asString(), stream););
                    // f.open((dir / (std::to_string(page) + ".json")).native());
                    // f << stream.rdbuf();
                    // f.close();
                    // stream.seekg(0);
                }
            } while (!stream.str().empty() && interrupted == false);
        } catch (const Json::Exception& e) {
            onJsonError(e);
        } catch (const HttpException& e) {
            std::cout << "HTTP error: " << e.what() << '\n';
        }
        semaphore.waitFor([&](std::size_t n) { return n == singleOrderHandlers.size(); });
    }

  private:
    class SingleOrderHandler : public Query {
      public:
        SingleOrderHandler(OrderHandler3& parent_, const Json::Value& o, std::size_t region)
            : parent(parent_), order(o), regionid(region) {
            id = o["id"].asUInt64();
            type = o["type"].asUInt64();
            station = o["stationID"].asUInt64();
            volume_entered = o["volumeEntered"].asUInt64();
            std::tm time = {};
            auto result = strptime(o["issued"].asString().c_str(), "%Y-%m-%dT%T", &time);
            if (result == nullptr)
                throw std::runtime_error("strptime failed on input '" + o["issued"].asString() + "'");
            start_date = timegm(&time);
            duration = o["duration"].asUInt64();
            range = o["range"].asString();
            price = o["price"].asDouble();
            volume = o["volume"].asUInt64();
            min_volume = o["minVolume"].asUInt64();
            buy = o["buy"].asBool();
        }

        void insOrder() {
            // insertOrder(writer.prepareStatement("insert or ignore into orders(orderid, typeid, station,
            // volume_entered, duration, min_volume) values(?, ?, ?, ?, ?, ?);")),
            auto& insertOrder = parent.insertOrder_;
            sqlite::Resetter resetter(insertOrder);
            sqlite::bind(insertOrder, 1, id);
            sqlite::bind(insertOrder, 2, type);
            sqlite::bind(insertOrder, 3, station);
            sqlite::bind(insertOrder, 4, volume_entered);
            sqlite::bind(insertOrder, 5, duration);
            sqlite::bind(insertOrder, 6, min_volume);
            sqlite::step(insertOrder);
        }

        void insBuyOrder() {
            // insertBuyOrder(writer.prepareStatement("insert or ignore into orders(orderid, range), values(?, ?);")),
            auto& insertBuyOrder = parent.insertBuyOrder_;
            sqlite::Resetter resetter(insertBuyOrder);
            sqlite::bind(insertBuyOrder, 1, id);
            sqlite::bind(insertBuyOrder, 2, range);
            sqlite::step(insertBuyOrder);
        }

        void insSellOrder() {
            // insertSellOrder(writer.prepareStatement("insert or ignore into sell_orders(orderid) values(?);")),
            auto& insertSellOrder = parent.insertSellOrder_;
            sqlite::Resetter resetter(insertSellOrder);
            sqlite::bind(insertSellOrder, 1, id);
            sqlite::step(insertSellOrder);
        }

        void insOrderView() {
            // insertOrderView(writer.prepareStatement("insert into orders(viewtime, orderid, price, volume)
            // values(strftime('%s', 'now'), ?, ?, ?);")) {}
            auto& insertOrderView = parent.insertOrderView_;
            sqlite::Resetter resetter(insertOrderView);
            sqlite::bind(insertOrderView, 1, id);
            sqlite::bind(insertOrderView, 2, price);
            sqlite::bind(insertOrderView, 3, volume);
            sqlite::bind(insertOrderView, 4, start_date);
            sqlite::step(insertOrderView);
        }

        void run(const sqlite::dbPtr&) override {
            step = 0;
            insOrder();
            step = 1;
            if (buy)
                insBuyOrder();
            else
                insSellOrder();
            step = 2;
            insOrderView();
            step = 3;
            finalize();
        }

        void onSqlError(const std::exception& e) override {
            std::cout << "SQL error: Failed to update order for item " << type << " (step " << step << "):\n"
                      << e.what() << '\n';
            std::cout << "order: \n" << order << '\n';
            finalize();
        }

        void finalize() { parent.finalize(); }

      private:
        OrderHandler3& parent;
        Json::Value order;
        std::size_t regionid;
        std::size_t id;
        std::size_t type;
        std::size_t station;
        std::size_t volume_entered;
        time_t start_date;
        std::size_t duration;
        std::string range;
        double price;
        std::size_t volume;
        std::size_t min_volume;
        bool buy;
        std::size_t step;
    };

    std::stringstream stream;
    Json::Value orders;
    std::size_t region;
    DatabaseConnection writer;
    sqlite::stmtPtr insertOrder_;
    sqlite::stmtPtr insertBuyOrder_;
    sqlite::stmtPtr insertSellOrder_;
    sqlite::stmtPtr insertOrderView_;
    Semaphore semaphore;
    bool print_progress;
    std::vector<SingleOrderHandler> singleOrderHandlers;
};

int main(int argc, char** args) {
    std::signal(SIGINT, &sigint_handler);
    std::size_t n;
    if (argc < 2) {
        std::cout << "Missing print_progress parameter\n";
        return -1;
    }
    char* end = nullptr;
    bool print_progress = std::strtoull(args[1], &end, 10);
    if (end == args[1]) {
        std::cout << "invalid print_progress parameter: " << args[1] << '\n';
        return -1;
    } else
        std::cout << "print_progress: " << print_progress << '\n';

    if (argc > 2) {
        n = std::strtoull(args[2], &end, 10);
        if (end == args[2]) {
            n = 30;
            std::cout << "invalid job count: " << args[2] << '\n';
        } else
            std::cout << "job count: " << n << '\n';
    } else {
        n = 30;
        std::cout << "job count: " << n << " (default)\n";
    }

    std::size_t region;
    if (argc > 3) {
        region = std::strtoull(args[3], &end, 10);
        if (end == args[3]) {
            region = 10000002;
            std::cout << "invalid region: " << args[3] << '\n';
        } else
            std::cout << "region: " << region << '\n';
    } else {
        region = 10000002;
        std::cout << "region: " << region << " (default)\n";
    }

    std::vector<std::size_t> types;
    if (argc > 4) {
        auto type = std::strtoull(args[4], &end, 10);
        if (end == args[4]) {
            std::cout << "invalid type: " << args[4] << '\n';
        } else {
            std::cout << "type: " << type << '\n';
            types.push_back(type);
        }
    } else {
        std::cout << "updating all types\n";
    }
    // auto access_token = getAccessToken();
    // std::cout << "Access Token: " << access_token << '\n';
    // auto prices = loadPrices();
    // assertTypes(db, prices);
    // saveOrders(outdatedHistories(db)/*, access_token*/);
    try {
        // saveHistoriesB(db, outdatedHistories(db)/*, access_token*/);
        // OrderHandler2 h(region, n, print_progress);
        OrderHandler3 h(region, print_progress);
        auto db = sqlite::getDB("/home/racko/btsync/git/emdr/market_history.db");
        auto select_stations = "select stationid from regions join constellations using (regionid) join systems using "
                               "(constellationid) join stations using (systemid) where regionid = " +
                               std::to_string(region);
        // auto select_types = std::string("select typeid from types natural left join (select typeid, max(viewtime) as
        // viewtime from orders join (" + select_stations + ") on orders.station = stationid natural join order_views
        // group by typeid) where viewtime - strftime('%s', 'now', '-5 minutes') <= 0"); auto set_inactive =
        // sqlite::prepare(db, types.empty() ? "update orders set active = 0 where typeid in (" + select_types + ") and
        // station in (" + select_stations + ");" : "update orders set active = 0 where typeid = " +
        // std::to_string(types[0]) + " and station in (" + select_stations + ");");
        auto set_inactive =
            sqlite::prepare(db, "update orders set active = 0 where station in (" + select_stations + ");");
        sqlite::step(set_inactive);
        // h.saveOrders(types.empty() ? outdatedOrders2(db, region) : types);
        h.updateOrders();
    } catch (const std::exception& e) {
        std::cout << "error: " << e.what() << '\n';
    }
    return 0;
}
