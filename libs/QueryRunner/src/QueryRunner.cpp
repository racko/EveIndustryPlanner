#include "QueryRunner.h"
#include <boost/asio/io_service.hpp>

struct QueryRunner::impl {
    impl(const char* path, const std::string& query)
        : db(sqlite::getDB(path)), stmt(sqlite::prepare(db, query)), work(ioService), thread([&] { ioService.run(); }) {
    }

    ~impl() {
        ioService.stop();
        thread.join();
    }

    void postQuery(std::function<void(const sqlite::dbPtr&, const sqlite::stmtPtr&)> f) {
        ioService.post([&, f] {
            f(db, stmt);
            sqlite::reset(stmt);
        });
    }

    impl(const impl&) = delete;
    impl& operator=(const impl&) = delete;
    impl(impl&&) = delete;
    impl& operator=(impl&&) = delete;

    sqlite::dbPtr db;
    sqlite::stmtPtr stmt;
    boost::asio::io_service ioService;
    boost::asio::io_service::work work;
    std::thread thread;
};

QueryRunner::QueryRunner(const char* path, const std::string& query) : pimpl(new impl(path, query)) {}

QueryRunner::~QueryRunner() = default;

void QueryRunner::postQuery(std::function<void(const sqlite::dbPtr&, const sqlite::stmtPtr&)> f) {
    pimpl->postQuery(f);
}

void QueryRunnerGroup::postQuery(std::function<void(const sqlite::dbPtr&, const sqlite::stmtPtr&)> f) {
    handlers[next].postQuery(f);
    next = (next + 1) % handlers.size();
}

QueryRunnerGroup::QueryRunnerGroup(std::size_t n, const char* path, const std::string& query) : next() {
    for (auto i = 0u; i < n; ++i)
        handlers.emplace_back(path, query);
}

struct DatabaseConnection::impl {

    impl(const char* path) : db(sqlite::getDB(path)), work(ioService), thread([&] { ioService.run(); }) {}

    ~impl() {
        ioService.stop();
        thread.join();
    }

    void withConnection(const std::shared_ptr<Query>& q) {
        ioService.post([&, q] {
            try {
                q->run(db);
            } catch (const std::exception& e) {
                q->onSqlError(e);
            }
        });
    }

    void withConnection(Query& q) {
        ioService.post([&] {
            try {
                q.run(db);
            } catch (const std::exception& e) {
                q.onSqlError(e);
            }
        });
    }

    sqlite::stmtPtr prepareStatement(const std::string& query) const { return sqlite::prepare(db, query); }

    impl(const impl&) = delete;
    impl& operator=(const impl&) = delete;
    impl(impl&&) = delete;
    impl& operator=(impl&&) = delete;

    sqlite::dbPtr db;
    boost::asio::io_service ioService;
    boost::asio::io_service::work work;
    std::thread thread;
};

DatabaseConnection::DatabaseConnection(const char* path) : pimpl(new impl(path)) {}
DatabaseConnection::~DatabaseConnection() = default;

void DatabaseConnection::withConnection(const std::shared_ptr<Query>& q) { pimpl->withConnection(q); }

void DatabaseConnection::withConnection(Query& q) { pimpl->withConnection(q); }

sqlite::stmtPtr DatabaseConnection::prepareStatement(const std::string& query) const {
    return pimpl->prepareStatement(query);
}
