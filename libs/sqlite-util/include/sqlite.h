#pragma once
#include <memory>
#include <string_view>

struct sqlite3;
struct sqlite3_stmt;

namespace sqlite {
using dbPtr = std::shared_ptr<sqlite3>;
using dbConstPtr = std::shared_ptr<const sqlite3>;

dbPtr connectDB(const char* name);
// calls connectDB and sets a number of properties to try and achieve maximum performance
dbPtr getDB(const char* path);

using stmtPtr = std::shared_ptr<sqlite3_stmt>;
using stmtConstPtr = std::shared_ptr<const sqlite3_stmt>;
stmtPtr prepare(const dbPtr& db, const std::string_view& sql);

int step(const stmtPtr& stmt);

void reset(const stmtPtr& stmt);

template <typename T>
void bind(const stmtPtr& stmt, int i, const T& v);
void bind_null(const stmtPtr& stmt, int i);

template <typename T>
T column(const stmtPtr& stmt, int i);

struct Resetter {
    stmtPtr stmt;
    Resetter(const stmtPtr& stmt_) : stmt(stmt_) {}
    ~Resetter() { reset(stmt); }
};

struct Transaction {
    stmtPtr beginStmt, endStmt, rollbackStmt;
    bool rollback = true;
    Transaction(const stmtPtr& beginStmt_, const stmtPtr& endStmt_, const stmtPtr& rollbackStmt_)
        : beginStmt(beginStmt_), endStmt(endStmt_), rollbackStmt(rollbackStmt_) {
        step(beginStmt);
        reset(beginStmt);
    }

    void commit() {
        step(endStmt);
        reset(endStmt);
        rollback = false;
    }

    ~Transaction() {
        if (rollback) {
            step(rollbackStmt);
            reset(rollbackStmt);
        }
    }
};

extern const int ROW;
extern const int DONE;
const char* errstr(int);
int64_t last_insert_rowid(const dbPtr&);
} // namespace sqlite
