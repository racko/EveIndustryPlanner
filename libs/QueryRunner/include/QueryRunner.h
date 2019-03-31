#pragma once
#include "sqlite.h"
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

class QueryRunner {
  public:
    QueryRunner(const char* path, const std::string& query);
    ~QueryRunner();

    void postQuery(std::function<void(const sqlite::dbPtr&, const sqlite::stmtPtr&)> f);

    QueryRunner(const QueryRunner&) = delete;
    QueryRunner& operator=(const QueryRunner&) = delete;
    QueryRunner(QueryRunner&&) = default;
    QueryRunner& operator=(QueryRunner&&) = default;

  private:
    struct impl;
    std::unique_ptr<impl> pimpl;
};

class QueryRunnerGroup {
  public:
    QueryRunnerGroup(size_t n, const char* path, const std::string& query);

    void postQuery(std::function<void(const sqlite::dbPtr&, const sqlite::stmtPtr&)> f);

    QueryRunnerGroup(const QueryRunnerGroup&) = delete;
    QueryRunnerGroup& operator=(const QueryRunnerGroup&) = delete;
    QueryRunnerGroup(QueryRunnerGroup&&) = default;
    QueryRunnerGroup& operator=(QueryRunnerGroup&&) = default;

  private:
    size_t next;
    std::vector<QueryRunner> handlers;
};

class Query {
  public:
    virtual void run(const sqlite::dbPtr& db) = 0;
    virtual void onSqlError(const std::exception&) = 0;
    virtual ~Query() = default;
};

class DatabaseConnection {
  public:
    DatabaseConnection(const char* path);
    ~DatabaseConnection();

    void withConnection(const std::shared_ptr<Query>& q);
    void withConnection(Query& q);

    sqlite::stmtPtr prepareStatement(const std::string& query) const;

    DatabaseConnection(const DatabaseConnection&) = delete;
    DatabaseConnection& operator=(const DatabaseConnection&) = delete;
    DatabaseConnection(DatabaseConnection&&) = default;
    DatabaseConnection& operator=(DatabaseConnection&&) = default;

  private:
    struct impl;
    std::unique_ptr<impl> pimpl;
};
