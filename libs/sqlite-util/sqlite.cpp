#include "libs/sqlite-util/sqlite.h"
#include <boost/numeric/conversion/cast.hpp>
#include <cassert>
#include <iostream>
#include <sqlite3.h>

namespace {
int to_int(std::size_t n) {
    assert(n <= static_cast<std::size_t>(std::numeric_limits<int>::max()));
    return static_cast<int>(n);
}
} // namespace

namespace sqlite {
using dbPtr = std::shared_ptr<sqlite3>;
using dbConstPtr = std::shared_ptr<const sqlite3>;
dbPtr connectDB(const char* name) {
    sqlite3* db{};
    auto rc = sqlite3_open(name, &db);
    if (rc) {
        sqlite3_close(db);
        throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + ": " + sqlite3_errmsg(db));
    }
    return std::shared_ptr<sqlite3>(db, sqlite3_close);
}

dbPtr getDB(const char* path) {
    auto db = connectDB(path);
    sqlite3_extended_result_codes(db.get(), 1);
    for (const auto& pragma : {"pragma cache_size=2000000;",
                               "pragma synchronous=OFF;",
                               "pragma temp_store=Memory;",
                               "pragma journal_mode=MEMORY;",
                               "PRAGMA wal_autocheckpoint=1000;",
                               "pragma foreign_keys=ON;"}) {
        step(sqlite::prepare(db, pragma));
    }
    return db;
}

using stmtPtr = std::shared_ptr<sqlite3_stmt>;
using stmtConstPtr = std::shared_ptr<const sqlite3_stmt>;
stmtPtr prepare(const dbPtr& db, const std::string_view& sql) {
    sqlite3_stmt* stmt{};
    const char* tail = nullptr;
    auto rc = sqlite3_prepare_v2(db.get(), sql.data(), to_int(sql.size()), &stmt, &tail);
    if (rc) {
        sqlite3_finalize(stmt);
        throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + ": " + sqlite3_errstr(rc) + " (" +
                                 std::to_string(rc) + ")");
    }
    if (tail != sql.data() + sql.size())
        std::cout << "Warning: sql statement was not fully consumed. Tail: \"" << tail << "\"\n";
    return std::shared_ptr<sqlite3_stmt>(stmt, sqlite3_finalize);
}

int step(const stmtPtr& stmt) {
    auto rc = sqlite3_step(stmt.get());
    // std::cout << "step returned " << rc << '\n';
    if (rc != SQLITE_ROW && rc != SQLITE_DONE)
        throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + ":  " + sqlite3_errstr(rc) + " (" +
                                 std::to_string(rc) + ")");
    return rc;
}

void reset(const stmtPtr& stmt) {
    // ignoring return code. step handles it.
    sqlite3_reset(stmt.get());
}

template <typename T>
struct bind_impl;

template <typename T>
void bind(const stmtPtr& stmt, int i, const T& v) {
    bind_impl<T>::bind(stmt, i, v);
}

void bind_null(const stmtPtr& stmt, int i) {
    // std::cout << "binding column " << i << " to null\n";
    auto rc = sqlite3_bind_null(stmt.get(), i);
    // std::cout << "bind returned " << rc << '\n';
    if (rc)
        throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + ":  " + sqlite3_errstr(rc) + " (" +
                                 std::to_string(rc) + ")");
}

template <>
struct bind_impl<std::string> {
    static void bind(const stmtPtr& stmt, int i, const std::string& v) {
        // std::cout << "binding column " << i << " to value \"" << v << "\"\n";
        auto rc = sqlite3_bind_text(stmt.get(), i, v.data(), to_int(v.size()), SQLITE_STATIC); // SQLITE_TRANSIENT?
        // std::cout << "bind returned " << rc << '\n';
        if (rc)
            throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + ":  " + sqlite3_errstr(rc) + " (" +
                                     std::to_string(rc) + ")");
    }
};

template <>
struct bind_impl<std::uint64_t> {
    static void bind(const stmtPtr& stmt, int i, std::uint64_t v) {
        // std::cout << "binding column " << i << " to value \"" << v << "\"\n";
        auto rc = sqlite3_bind_int64(stmt.get(), i, boost::numeric_cast<std::int64_t>(v));
        // std::cout << "bind returned " << rc << '\n';
        if (rc)
            throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + ":  " + sqlite3_errstr(rc) + " (" +
                                     std::to_string(rc) + ")");
    }
};

template <>
struct bind_impl<std::uint32_t> {
    static void bind(const stmtPtr& stmt, int i, std::uint32_t v) {
        // std::cout << "binding column " << i << " to value \"" << v << "\"\n";
        auto rc = sqlite3_bind_int64(stmt.get(), i, std::int64_t(v));
        // std::cout << "bind returned " << rc << '\n';
        if (rc)
            throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + ":  " + sqlite3_errstr(rc) + " (" +
                                     std::to_string(rc) + ")");
    }
};

template <typename T>
struct column_impl;

template <typename T>
T column(const stmtPtr& stmt, int i) {
    return column_impl<T>::column(stmt, i);
}

#define IMPLEMENT_COLUMN(cpp_type, sqlite_type)                                                                        \
    template <>                                                                                                        \
    struct column_impl<cpp_type> {                                                                                     \
        static cpp_type column(const stmtPtr& stmt, int i) {                                                           \
            auto result = sqlite3_column_##sqlite_type(stmt.get(), i);                                                 \
            return result;                                                                                             \
        }                                                                                                              \
    };                                                                                                                 \
    template cpp_type column<cpp_type>(const stmtPtr&, int);

IMPLEMENT_COLUMN(double, double)
IMPLEMENT_COLUMN(std::int32_t, int)
IMPLEMENT_COLUMN(std::int64_t, int64)
IMPLEMENT_COLUMN(const std::uint8_t*, text)

template <>
struct column_impl<std::string> {
    static std::string column(const stmtPtr& stmt, int i) {
        // this reinterpret_cast to const char* is safe according to [basic.lval] 8.8 (C++ standard):
        // If a program attempts to access the stored value of an object through a glvalue of other than one of the
        // following types the behavior is undefined:
        // ... a char, unsigned char, or std::byte type.
        // (see https://stackoverflow.com/a/16261758)
        return std::string(reinterpret_cast<const char*>(sqlite::column<const std::uint8_t*>(stmt, i)));
    }
};
template std::string column<std::string>(const stmtPtr&, int);

#define IMPLEMENT_BIND(cpp_type, sqlite_type)                                                                          \
    template <>                                                                                                        \
    struct bind_impl<cpp_type> {                                                                                       \
        static void bind(const stmtPtr& stmt, int i, const cpp_type& v) {                                              \
            auto rc = sqlite3_bind_##sqlite_type(stmt.get(), i, v);                                                    \
            if (rc)                                                                                                    \
                throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + ":  " + sqlite3_errstr(rc) + " (" +        \
                                         std::to_string(rc) + ")");                                                    \
        }                                                                                                              \
    };                                                                                                                 \
    template void bind<cpp_type>(const stmtPtr&, int, const cpp_type&);

IMPLEMENT_BIND(double, double)
IMPLEMENT_BIND(float, double)
IMPLEMENT_BIND(bool, int)
IMPLEMENT_BIND(std::int16_t, int)
IMPLEMENT_BIND(std::int32_t, int)
IMPLEMENT_BIND(std::int64_t, int64)

template void bind<std::size_t>(const stmtPtr&, int, const std::size_t&);
template void bind<std::string>(const stmtPtr&, int, const std::string&);

const int ROW = SQLITE_ROW;
const int DONE = SQLITE_DONE;
const char* errstr(int code) { return sqlite3_errstr(code); }
int64_t last_insert_rowid(const dbPtr& db) { return sqlite3_last_insert_rowid(db.get()); }
} // namespace sqlite
