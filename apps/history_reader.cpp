#include "Finalizer.h"
#include "HTTPSRequestHandler.h"
#include "QueryRunner.h"
#include "Semaphore.h"
#include "access_token.h"
#include "outdatedHistories.h"
#include "sqlite.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <json/json.h>
#include <list>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <unordered_set>

static constexpr const char* ANSI_CLEAR_LINE = "\033[2K";

// template<typename Functor>
// void handleHistory(DatabaseConnection& writer, const sqlite::stmtPtr& insertHistory, std::size_t typeID, const
// Json::Value& history, Functor f) {
//    const auto& items = history["items"];
//    if (items.isNull()) {
//        f();
//        return;
//    }
//    Semaphore semaphore;
//    for (const auto& item : items) {
//        std::tm time = {};
//        auto result = strptime(item["date"].asString().c_str(), "%Y-%m-%d", &time);
//        if (result == nullptr)
//            throw std::runtime_error("parse failed");
//        std::uint64_t volume = item["volume"].asUInt64();
//        auto then_time_t = timegm(&time);
//        std::uint64_t orderCount = item["orderCount"].asUInt64();
//        auto avg = item["avgPrice"].asDouble();
//        auto low = item["lowPrice"].asDouble();
//        auto high = item["highPrice"].asDouble();
//        writer.withConnection([=, &semaphore] (const sqlite::dbPtr& db) {
//            sqlite::bind(insertHistory, 1, typeID);
//            sqlite::bind(insertHistory, 2, then_time_t);
//            sqlite::bind(insertHistory, 3, 10000002);
//            sqlite::bind(insertHistory, 4, orderCount);
//            sqlite::bind(insertHistory, 5, low);
//            sqlite::bind(insertHistory, 6, high);
//            sqlite::bind(insertHistory, 7, avg);
//            sqlite::bind(insertHistory, 8, volume);
//            sqlite::step(insertHistory);
//            sqlite::reset(insertHistory);
//            semaphore.release();
//        });
//    }
//    semaphore.acquire(items.size());
//    f();
//}
//
// template<typename Functor>
// void updateLastUpdate(DatabaseConnection& writer, const sqlite::stmtPtr& update, std::size_t typeID, Functor f) {
//    Semaphore semaphore;
//    writer.withConnection([&] (const sqlite::dbPtr& db) {
//            sqlite::bind(update, 1, typeID);
//            sqlite::step(update);
//            sqlite::reset(update);
//            semaphore.release();
//            });
//    semaphore.acquire();
//    f();
//}

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

bool interrupted = false;
void sigint_handler(int) {
    // interrupted = true;
    for (auto h : sigint_handlers)
        h();
}

int32_t to_int32(std::size_t n) {
    assert(n <= static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()));
    return static_cast<std::int32_t>(n);
}

uint32_t to_uint32(std::size_t n) {
    assert(n <= static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()));
    return static_cast<std::uint32_t>(n);
}

class HistoryHandler {
  public:
    HistoryHandler()
        : writer("market_history.db"),
          insertStmt_(writer.prepareStatement("insert or ignore into history values(?, ?, ?, ?, ?, ?, ?, ?);")),
          updateStmt_(writer.prepareStatement(
              "insert or replace into last_history_update values(?, 10000002, strftime('%s', 'now'));")),
          http_handlers(15, "crest-tq.eveonline.com", 443) {}

    void saveHistories(const std::vector<std::size_t>& types /*, const std::string& access_token*/) {
        auto handler_handle = addSigintHandler([&] { semaphore.setTo(to_int32(types.size())); });
        auto sigintHandlerRemover = makeFinalizer([&] { removeSigintHandler(handler_handle); });
        total = types.size();
        for (auto p : types) {
            requests.emplace_back(std::make_unique<HistoryRequest>(
                "/market/10000002/history/?type=https://crest-tq.eveonline.com/inventory/types/" + std::to_string(p) +
                    "/",
                *this, p));
            http_handlers.postRequest(*requests.back(), "");
        }
        semaphore.acquire(to_uint32(types.size()));
    }

  private:
    auto releaseOne() {
        semaphore.release();
        return semaphore.get();
    }

    class HistoryRequest : public RequestWithResponseHandler {
      public:
        HistoryRequest(const std::string& uri, HistoryHandler& parent_, std::size_t type)
            : RequestWithResponseHandler(uri), parent(parent_), t(type) {}

        ~HistoryRequest() {
            if (!queries.empty())
                lock.acquire(queries.size() + 1);
        }

        void handleResponse(const Response&, std::istream& s) override {
            std::stringstream ss;
            s >> responseBody.rdbuf();
            try {
                Json::Value root;
                responseBody >> root;
                items = root["items"];
            } catch (const std::exception& e) {
                onJsonError(e);
                items.clear();
            }
            queries.reserve(items.size());
            for (const auto& item : items) {
                try {
                    queries.emplace_back(*this, t, item);
                    parent.writer.withConnection(queries.back());
                } catch (const std::exception& e) {
                    onJsonError(e, item);
                }
            }
            lock.waitFor([&](std::size_t n) { return n == queries.size(); });
            UpdateQuery q(*this, t);
            parent.writer.withConnection(q);
            lock.waitFor([&](std::size_t n) { return n == queries.size() + 1; });
            finalize();
        }

        void onJsonError(const std::exception& e) {
            std::cout << "JSON error: Failed to update history for item " << t << ":\n" << e.what() << '\n';
            responseBody.seekg(0);
            std::cout << "response: \n" << responseBody.rdbuf() << '\n';
        }

        void onJsonError(const std::exception& e, const Json::Value& item) {
            std::cout << "JSON error: Failed to update history record for item " << t << ":\n" << e.what() << '\n';
            std::cout << "record: \n" << item << '\n';
        }

        void onHttpError(const std::exception& e) override {
            std::cout << "HTTP error: Failed to update history for item " << t << ":\n" << e.what() << '\n';
            finalize();
        }

        void finalize() {
            auto n = parent.releaseOne();
            if (parent.print_progress)
                std::cout << ANSI_CLEAR_LINE << " " << n << " / " << parent.total << " | "
                          << (double(n) / double(parent.total) * 100.0) << "%\r" << std::flush;
        }

      private:
        class HistoryQuery : public Query {
          public:
            HistoryQuery(HistoryRequest& parent_, std::size_t type_, const Json::Value& item_)
                : parent(parent_), type(type_), item(item_) {
                std::tm time{};
                auto result = strptime(item["date"].asString().c_str(), "%Y-%m-%d", &time);
                if (result == nullptr)
                    throw std::runtime_error("parse failed");
                volume = item["volume"].asUInt64();
                then_time_t = timegm(&time);
                orderCount = item["orderCount"].asUInt64();
                avg = item["avgPrice"].asDouble();
                low = item["lowPrice"].asDouble();
                high = item["highPrice"].asDouble();
            }
            void run(const sqlite::dbPtr&) override {
                auto& insertStmt = parent.parent.insertStmt_;
                sqlite::bind(insertStmt, 1, type);
                sqlite::bind(insertStmt, 2, then_time_t);
                sqlite::bind(insertStmt, 3, 10000002);
                sqlite::bind(insertStmt, 4, orderCount);
                sqlite::bind(insertStmt, 5, low);
                sqlite::bind(insertStmt, 6, high);
                sqlite::bind(insertStmt, 7, avg);
                sqlite::bind(insertStmt, 8, volume);
                sqlite::step(insertStmt);
                sqlite::reset(insertStmt);
                finalize();
            }

            void onSqlError(const std::exception& e) override {
                std::cout << "SQL error: Failed to update history record for item " << type << ":\n"
                          << e.what() << '\n';
                std::cout << "history record: \n" << item << '\n';
                finalize();
            }

            void finalize() { parent.lock.release(); }

          private:
            HistoryRequest& parent;
            std::size_t type;
            Json::Value item;
            std::uint64_t volume;
            time_t then_time_t;
            std::uint64_t orderCount;
            double avg;
            double low;
            double high;
        };
        class UpdateQuery : public Query {
          public:
            UpdateQuery(HistoryRequest& parent_, std::size_t type_) : parent(parent_), type(type_) {}
            void run(const sqlite::dbPtr&) override {
                auto& updateStmt = parent.parent.updateStmt_;
                sqlite::bind(updateStmt, 1, type);
                sqlite::step(updateStmt);
                sqlite::reset(updateStmt);
                finalize();
            }

            void onSqlError(const std::exception& e) override {
                std::cout << "SQL error: Failed to update last_history_update for item " << type << ":\n"
                          << e.what() << '\n';
                finalize();
            }

            void finalize() { parent.lock.release(); }

          private:
            HistoryRequest& parent;
            std::size_t type;
        };

        HistoryHandler& parent;
        std::vector<HistoryQuery> queries;
        std::size_t t;
        std::stringstream responseBody;
        Json::Value items;
        Semaphore lock;
    };

    DatabaseConnection writer;
    sqlite::stmtPtr insertStmt_;
    sqlite::stmtPtr updateStmt_;
    Semaphore semaphore;
    std::size_t total;
    std::vector<std::unique_ptr<HistoryRequest>> requests;
    bool print_progress = true;
    HTTPSRequestHandlerGroup http_handlers;
};

void saveHistoriesST(const sqlite::dbPtr& db, const std::vector<std::size_t>& types /*, const std::string& access_token*/) {
    auto insertStmt = sqlite::prepare(db, "insert or ignore into history values(?, ?, ?, ?, ?, ?, ?, ?);");
    auto updateStmt =
        sqlite::prepare(db, "insert or replace into last_history_update values(?, 10000002, strftime('%s', 'now'));");
    auto count = 0u;
    for (auto p : types) {
        if (interrupted) {
            return;
        }
        ++count;
        auto progressReport = makeFinalizer([&] {
            std::cout << ANSI_CLEAR_LINE << " " << count << " / " << types.size() << " | "
                      << (double(count) / double(types.size()) * 100.0) << "%\r" << std::flush;
        });
        try {
            std::stringstream body;
            simpleGet("https://crest-tq.eveonline.com/market/10000002/types/" + std::to_string(p) + "/history/", body);
            Json::Value history;
            body >> history;
            const auto& items = history["items"];
            if (items.isNull())
                continue;
            for (const auto& item : items) {
                std::tm time = {};
                auto result = strptime(item["date"].asString().c_str(), "%Y-%m-%d", &time);
                if (result == nullptr)
                    throw std::runtime_error("parse failed");
                std::uint64_t volume = item["volume"].asUInt64();
                auto then_time_t = timegm(&time);
                std::uint64_t orderCount = item["orderCount"].asUInt64();
                auto avg = item["avgPrice"].asDouble();
                auto low = item["lowPrice"].asDouble();
                auto high = item["highPrice"].asDouble();
                sqlite::bind(insertStmt, 1, p);
                sqlite::bind(insertStmt, 2, then_time_t);
                sqlite::bind(insertStmt, 3, 10000002);
                sqlite::bind(insertStmt, 4, orderCount);
                sqlite::bind(insertStmt, 5, low);
                sqlite::bind(insertStmt, 6, high);
                sqlite::bind(insertStmt, 7, avg);
                sqlite::bind(insertStmt, 8, volume);
                sqlite::step(insertStmt);
                sqlite::reset(insertStmt);

                sqlite::bind(updateStmt, 1, p);
                sqlite::step(updateStmt);
                sqlite::reset(updateStmt);
            }
        } catch (const std::exception& e) {
            std::cout << "item " << p << ": " << e.what() << '\n';
        }
    }
}

// void saveHistoriesB(const sqlite::dbPtr& db, const std::vector<std::size_t>& types/*, const std::string& access_token*/) {
//    DatabaseConnection writer("market_history.db");
//    auto insertStmt = writer.prepareStatement("insert or ignore into history values(?, ?, ?, ?, ?, ?, ?, ?);");
//    auto updateStmt = writer.prepareStatement("insert or replace into last_history_update values(?, 10000002,
//    strftime('%s', 'now'));"); auto handler_handle = addSigintHandler([&] { interrupted = true; }); auto
//    sigintHandlerRemover = makeFinalizer([&] { removeSigintHandler(handler_handle); }); auto count = 0u; for (auto p :
//    types) {
//        if (interrupted) {
//            return;
//        }
//        ++count;
//        try {
//            std::stringstream body;
//            simpleGet("https://crest-tq.eveonline.com/market/10000002/types/" + std::to_string(p) + "/history/",
//            body); Json::Value history; body >> history; handleHistory(writer, insertStmt, p, history, [&, p] {
//                updateLastUpdate(writer, updateStmt, p, [&] {
//                    std::cout << ANSI_CLEAR_LINE << " " << count << " / " << types.size() << " | " << (double(count) /
//                    double(types.size()) * 100.0) << "%\r" << std::flush;
//                });
//            });
//        } catch(const std::exception& e) {
//            std::cout << "item " << p << ": " << e.what() << '\n';
//        }
//    }
//}

auto outdatedHistories() { return outdatedHistories(sqlite::getDB("market_history.db")); }

int main() {
    std::cout << std::boolalpha;
    std::signal(SIGINT, &sigint_handler);
    // auto access_token = getAccessToken();
    // std::cout << "Access Token: " << access_token << '\n';
    // auto prices = loadPrices();
    // assertTypes(db, prices);
    try {
        // saveHistoriesB(db, outdatedHistories(db)/*, access_token*/);
        HistoryHandler h;
        h.saveHistories(outdatedHistories());
    } catch (...) {
        std::cout << "error\n";
    }
    return 0;
}
