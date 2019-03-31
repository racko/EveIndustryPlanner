#undef NDEBUG
#include <mutex>
#include <thread>
#include <iterator>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <chrono>
#include <fstream>
#include <ctime>
#include <cstdio>
#include <csignal>
#include <iomanip>
#include <vector>
#include <yaml-cpp/yaml.h>
#include <json/json.h>
#include <sstream>
#include "sqlite.h"
#include "LinearOptimizer.h"
#include <zmq.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/circular_buffer.hpp>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#define IMPLEMENT_GETTER(cpp_type, json_type) \
namespace rapidjson { \
  template<> \
  struct get_impl<cpp_type> { \
    static cpp_type get(const Value& v) { \
      if (!v.Is##json_type()) \
        throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + ": No " #json_type); \
       return v.Get##json_type(); \
     } \
  }; \
}


namespace rapidjson {

  template<typename T>
  struct get_impl;

  template<typename T> T get(const Value& v) {
    return get_impl<T>::get(v);
  }

  template<>
  struct get_impl<std::string> {
    static std::string get(const Value& v) {
      if (!v.IsString())
        throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + ": Not a string");
      return std::string(v.GetString(), v.GetStringLength());
    }
  };

  template<typename T>
  T getMember(const Value& v, const std::string& name) {
    if (!v.IsObject())
      throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + ": Not an object");
    auto member = v.FindMember(name.c_str());
    if (member == v.MemberEnd())
      throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + ": No member \"" + name + "\"");
    return get_impl<T>::get(member->value);
  }

  template<typename T>
  T get(const Value& v, std::size_t i) {
    if (!v.IsArray())
      throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + ": Not an array");
    if (v.Size() <= i)
      throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + ": Too small");
    return get_impl<T>::get(v[(SizeType(i))]);
  }

  template<typename T>
  std::vector<T> getArray(const Value& v) {
    if (!v.IsArray())
      throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + ": Not an array");
    std::vector<T> array;
    array.reserve(v.Size());
    for (auto it = v.Begin(); it != v.End(); ++it)
      array.push_back(get_impl<T>::get(*it));
    return array;
  }

  template<typename T>
  struct get_impl<std::vector<T>> {
    static std::vector<T> get(const Value& v) {
      return getArray<T>(v);
    }
  };
}

IMPLEMENT_GETTER(std::uint64_t, Uint64)
IMPLEMENT_GETTER(std::int64_t, Int64)
IMPLEMENT_GETTER(std::uint32_t, Uint)
IMPLEMENT_GETTER(std::int32_t, Int)
IMPLEMENT_GETTER(double, Double)
IMPLEMENT_GETTER(bool, Bool)

void printJSON(std::ostream& s, const rapidjson::Value& v, std::size_t indent) {
    switch (v.GetType()) {
    case rapidjson::kNullType:
        s << "Null";
        break;
    case rapidjson::kObjectType:
        {
            s << "{\n";
            auto newIndent = indent + 1;
            auto it = v.MemberBegin();
            if (it != v.MemberEnd()) {
                std::fill_n(std::ostream_iterator<char>(s), 2*newIndent, ' ');
                s << "\"" << it->name.GetString() << "\": ";
                printJSON(s, (it++)->value, newIndent);
                for (; it != v.MemberEnd(); ++it) {
                    s << ",\n";
                    std::fill_n(std::ostream_iterator<char>(s), 2*newIndent, ' ');
                    s << "\"" << it->name.GetString() << "\": ";
                    printJSON(s, it->value, newIndent);
                }
            }
            s << "\n";
            std::fill_n(std::ostream_iterator<char>(s), 2*indent, ' ');
            s << "}";
        }

        break;
    case rapidjson::kArrayType:
        s << "[";
        if (!v.Empty()) {
            auto it = v.Begin();
            printJSON(s, *it++, indent+1);
            for (; it != v.End(); ++it) {
                s << ", ";
                printJSON(s, *it, indent+1);
            }
        }
        s << "]";
        break;
    case rapidjson::kNumberType:
        if (v.IsDouble()) {
            s << v.GetDouble();
        } else if (v.IsInt()) {
            s << v.GetInt();
        } else if (v.IsInt64()) {
            s << v.GetInt64();
        } else if (v.IsUint()) {
            s << v.GetUint();
        } else if (v.IsUint64()) {
            s << v.GetUint64();
        }
        break;
    case rapidjson::kTrueType:
        s << "true";
        break;
    case rapidjson::kFalseType:
        s << "false";
        break;
    case rapidjson::kStringType:
        s << "\"" << v.GetString() << "\"";
        break;
    }
}

struct Key {
  std::string name;
  std::string key;
  Key(const std::string& name_, const std::string& key_) : name(name_), key(key_) {}
};

struct Generator {
  std::string name;
  std::string version;
  Generator(const std::string& name_, const std::string& version_) : name(name_), version(version_) {}
};

using Time = std::chrono::system_clock::time_point;
using nanos = std::chrono::nanoseconds;

template<typename T>
struct Rowset {
  std::vector<T> x;
  Time generatedAt;
  std::uint64_t regionID;
  std::uint64_t typeID;

  template<typename T1>
  Rowset(T1&& x_, Time g, std::uint64_t r, std::uint64_t t) : x(x_), generatedAt(g), regionID(r), typeID(t) {}
};

template<typename T>
struct Message {
  std::vector<Key> uploadKeys;
  Generator generator;
  Time currentTime;
  std::string version;
  std::vector<Rowset<T>> rowsets;

  template<typename T1>
  Message(const std::vector<Key>& k_, const Generator& g_, const Time t_, const std::string& v_, T1&& r_) : uploadKeys(k_), generator(g_), currentTime(t_), version(v_), rowsets(std::forward<T1>(r_)) {}
};

struct Orders {
  double price;
  std::uint64_t volRemaining;
  std::int16_t range;
  std::uint64_t orderID;
  std::uint64_t volumeEntered;
  std::uint64_t minVolume;
  bool bid;
  Time issueDate;
  std::chrono::system_clock::duration duration;
  std::uint64_t stationID;
  std::uint64_t solarSystemID;

  Orders(double price_, std::uint64_t volRemaining_, std::int16_t range_, std::uint64_t orderID_, std::uint64_t volumeEntered_, std::uint64_t minVolume_, bool bid_, Time issueDate_, std::chrono::system_clock::duration duration_, std::uint64_t stationID_, std::uint64_t solarSystemID_) : price(price_), volRemaining(volRemaining_), range(range_), orderID(orderID_), volumeEntered(volumeEntered_), minVolume(minVolume_), bid(bid_), issueDate(issueDate_), duration(duration_), stationID(stationID_), solarSystemID(solarSystemID_) {}

};

struct History {
  Time date;
  std::uint64_t orders;
  std::uint64_t quantity;
  double low;
  double high;
  double average;

  History(Time date_, std::uint64_t orders_, std::uint64_t quantity_, double low_, double high_, double average_) : date(date_), orders(orders_), quantity(quantity_), low(low_), high(high_), average(average_) {}
};

namespace rapidjson {
  template<>
  struct get_impl<Key> {
    static Key get(const Value& v) {
      auto name = getMember<std::string>(v, "name");
      auto key = getMember<std::string>(v, "key");
      return Key(name, key);
    }
  };

  template<>
  struct get_impl<Generator> {
    static Generator get(const Value& v) {
      auto name = getMember<std::string>(v, "name");
      auto version = getMember<std::string>(v, "version");
      return Generator(name, version);
    }
  };

  template<>
  struct get_impl<Time> {
    static Time get(const Value& v) {
      auto timeStr = rapidjson::get<std::string>(v);
      tm timeTm{};
      auto result = strptime(timeStr.c_str(), "%FT%T%z", &timeTm); 
      // if (result < timeStr.c_str() || result - timeStr.c_str() != timeStr.size())
      if (result == nullptr)
        throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + ": strptime failed");
      return Time(std::chrono::seconds(timegm(&timeTm)));
    }
  };

  template<typename T>
  struct get_impl<Rowset<T>> {
    static Rowset<T> get(const Value& v) {
      auto regionID = getMember<std::uint64_t>(v, "regionID");
      auto typeID = getMember<std::uint64_t>(v, "typeID");
      auto generatedAt = getMember<Time>(v, "generatedAt");
      return Rowset<T>(getMember<std::vector<T>>(v, "rows"), generatedAt, regionID, typeID);
    }
  };

  template<>
  struct get_impl<History> {
    static History get(const Value& v) {
      using rapidjson::get;
      // TODO: read column order from columns field in message
      auto date = get<Time>(v,0);
      auto orders = get<std::uint64_t>(v,1);
      auto quantity = get<std::uint64_t>(v,2);
      auto low = get<double>(v,3);
      auto high = get<double>(v,4);
      auto average = get<double>(v,5);
      return History(date, orders, quantity, low, high, average);
    }
  };

  template<>
  struct get_impl<Orders> {
    static Orders get(const Value& v) {
      using rapidjson::get;
      // TODO: read column order from columns field in message
      auto price = get<double>(v, 0);
      auto volRemaining = get<std::uint64_t>(v, 1);
      auto range = get<std::int32_t>(v, 2);
      auto orderID = get<std::uint64_t>(v, 3);
      auto volumeEntered = get<std::uint64_t>(v, 4);
      auto minVolume = get<std::uint64_t>(v, 5);
      auto bid = get<bool>(v, 6);
      auto issueDate = get<Time>(v, 7);
      auto duration = get<std::uint32_t>(v, 8) * std::chrono::hours(24);
      auto stationID = get<std::uint64_t>(v, 9);
      auto solarSystemID = get<std::uint64_t>(v, 10);
      return Orders(price, volRemaining, range, orderID, volumeEntered, minVolume, bid, issueDate, duration, stationID, solarSystemID);
    }
  };

  template<typename T>
  struct get_impl<Message<T>> {
    static Message<T> get(const Value& v) {
      auto uploadKeys = getMember<std::vector<Key>>(v, "uploadKeys");
      auto generator = getMember<Generator>(v, "generator");
      auto currentTime = getMember<Time>(v, "currentTime");
      auto version = getMember<std::string>(v, "version");
      auto columns = getMember<std::vector<std::string>>(v, "columns");
      auto rowsets = getMember<std::vector<Rowset<T>>>(v, "rowsets");
      return Message<T>(uploadKeys, generator, currentTime, version, rowsets);
    }
  };
}


struct emdr_db {
  sqlite::dbPtr db;
  sqlite::stmtPtr insertOrdersStmt;
  sqlite::stmtPtr insertHistoryStmt;
  sqlite::stmtPtr insertRowsetHeaderStmt;
  sqlite::stmtPtr insertMessageHeaderStmt;
  sqlite::stmtPtr insertGeneratorStmt;
  sqlite::stmtPtr insertUploadKeyStmt;
  sqlite::stmtPtr insertUploadKeyAssociationStmt;
  sqlite::stmtPtr getUploadKeyStmt;
  sqlite::stmtPtr getGeneratorStmt;
  sqlite::stmtPtr databaseListStmt;
  sqlite::stmtPtr beginTransactionStmt;
  sqlite::stmtPtr endTransactionStmt;
  std::ofstream backlog;

  static constexpr const auto insertOrdersSQL = "insert into orders values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
  static constexpr const auto insertHistorySQL = "insert into history values (?, ?, ?, ?, ?, ?, ?);";
  static constexpr const auto insertRowsetHeaderSQL = "insert into rowset_headers values (null, ?, ?, ?, ?);";
  static constexpr const auto insertMessageHeaderSQL = "insert into message_headers values (null, ?, ?, ?);";
  static constexpr const auto insertGeneratorSQL = "insert or ignore into generators values (null, ?, ?);";
  static constexpr const auto insertUploadKeySQL = "insert or ignore into upload_keys values (null, ?, ?);";
  static constexpr const auto insertUploadKeyAssociationSQL = "insert into upload_key_associations values (?, ?);";

  static constexpr const auto getGeneratorSQL = "select id from generators where name = ? and version = ?;";
  static constexpr const auto getUploadKeySQL = "select id from upload_keys where name = ? and key = ?;";
  static constexpr const auto databaseListSQL = "PRAGMA database_list;";
  static constexpr const auto beginTransactionSQL = "begin;";
  static constexpr const auto endTransactionSQL = "end;";

  emdr_db(const char* name) : db(sqlite::connectDB(name))
                              , insertOrdersStmt(sqlite::prepare(db, insertOrdersSQL))
                              , insertHistoryStmt(sqlite::prepare(db, insertHistorySQL))
                              , insertRowsetHeaderStmt(sqlite::prepare(db, insertRowsetHeaderSQL))
                              , insertMessageHeaderStmt(sqlite::prepare(db, insertMessageHeaderSQL))
                              , insertGeneratorStmt(sqlite::prepare(db, insertGeneratorSQL))
                              , insertUploadKeyStmt(sqlite::prepare(db, insertUploadKeySQL))
                              , insertUploadKeyAssociationStmt(sqlite::prepare(db, insertUploadKeyAssociationSQL))
                              , getUploadKeyStmt(sqlite::prepare(db, getUploadKeySQL))
                              , getGeneratorStmt(sqlite::prepare(db, getGeneratorSQL))
                              , databaseListStmt(sqlite::prepare(db, databaseListSQL))
                              , beginTransactionStmt(sqlite::prepare(db, beginTransactionSQL))
                              , endTransactionStmt(sqlite::prepare(db, endTransactionSQL))
                              , backlog("/tmp/backlog.json") {
    for (const auto& pragma : { "pragma cache_size=2000000;", "pragma synchronous=Full;", "pragma temp_store=Memory;", "pragma journal_mode=DELETE;", "PRAGMA wal_autocheckpoint=1000;" }) {
      int rc;
      do {
        sqlite::step(sqlite::prepare(db, pragma));
      } while(rc == SQLITE_ROW);
    }
    auto rc = sqlite3_extended_result_codes(db.get(), 1);
    if (rc)
      std::cout << __PRETTY_FUNCTION__ << ":  " << sqlite3_errstr(rc) << " (" << rc << ")\n";
  }

  std::size_t insertGenerator(const Generator& g) {
    using namespace sqlite;
    // std::cout << __PRETTY_FUNCTION__ << '\n';
    sqlite::bind(getGeneratorStmt, 1, g.name);
    sqlite::bind(getGeneratorStmt, 2, g.version);
    auto rc = step(getGeneratorStmt);
    sqlite::Resetter getResetter(getGeneratorStmt);
    if (rc == SQLITE_ROW) {
      // std::cout << "generator already known\n";
      return column<std::int64_t>(getGeneratorStmt, 0);
    }
    // std::cout << "Inserting new generator\n";
    sqlite::bind(insertGeneratorStmt, 1, g.name);
    sqlite::bind(insertGeneratorStmt, 2, g.version);
    step(insertGeneratorStmt);
    sqlite::Resetter insertResetter(insertGeneratorStmt);
    return sqlite3_last_insert_rowid(db.get());
  }
  
  std::size_t insertUploadKey(const Key& k) {
    // std::cout << __PRETTY_FUNCTION__ << '\n';
    using namespace sqlite;
    sqlite::bind(getUploadKeyStmt, 1, k.name);
    sqlite::bind(getUploadKeyStmt, 2, k.key);
    auto rc = step(getUploadKeyStmt);
    sqlite::Resetter getResetter(getUploadKeyStmt);
    if (rc == SQLITE_ROW)
      return column<std::int64_t>(getUploadKeyStmt, 0);
    sqlite::bind(insertUploadKeyStmt, 1, k.name);
    sqlite::bind(insertUploadKeyStmt, 2, k.key);
    step(insertUploadKeyStmt);
    sqlite::Resetter insertResetter(insertUploadKeyStmt);
    return sqlite3_last_insert_rowid(db.get());
  }
  
  template<typename T>
  struct insert_impl;
  
  template<typename T>
  const sqlite::stmtPtr& getStmt();
  
  template<typename T>
  void insert(const T& x, std::size_t headerID) {
    insert_impl<T>::insert(getStmt<T>(), x, headerID);
  }
  
  void associateUploadKey(std::size_t uploadKeyID, std::size_t messageID) {
    using namespace sqlite;
    // std::cout << __PRETTY_FUNCTION__ << '\n';
    sqlite::bind(insertUploadKeyAssociationStmt, 1, uploadKeyID);
    sqlite::bind(insertUploadKeyAssociationStmt, 2, messageID);
    step(insertUploadKeyAssociationStmt);
    sqlite::Resetter resetter(insertUploadKeyAssociationStmt);
  }
  
  template<typename T>
  std::size_t insertRowsetHeader(const Rowset<T>& r, std::size_t messageID) {
    using namespace sqlite;
    // std::cout << __PRETTY_FUNCTION__ << '\n';
    sqlite::bind(insertRowsetHeaderStmt, 1, messageID);
    sqlite::bind(insertRowsetHeaderStmt, 2, r.generatedAt.time_since_epoch().count());
    sqlite::bind(insertRowsetHeaderStmt, 3, r.regionID);
    sqlite::bind(insertRowsetHeaderStmt, 4, r.typeID);
    step(insertRowsetHeaderStmt);
    sqlite::Resetter resetter(insertRowsetHeaderStmt);
    return sqlite3_last_insert_rowid(db.get());
  }
  
  template<typename T>
  void insertRowset(const Rowset<T>& r, std::size_t messageID) {
    // std::cout << __PRETTY_FUNCTION__ << '\n';
    auto headerID = insertRowsetHeader(r, messageID);
    for (const auto& row : r.x)
      insert<T>(row, headerID);
  }
  
  template<typename T>
  std::size_t insertMessageHeader(const Message<T>& m) {
    // std::cout << __PRETTY_FUNCTION__ << '\n';
    using namespace sqlite;
    auto generatorID = insertGenerator(m.generator);
  
    sqlite::bind(insertMessageHeaderStmt, 1, m.currentTime.time_since_epoch().count());
    sqlite::bind(insertMessageHeaderStmt, 2, m.version);
    sqlite::bind(insertMessageHeaderStmt, 3, generatorID);
    step(insertMessageHeaderStmt);
    sqlite::Resetter resetter(insertMessageHeaderStmt);
    return sqlite3_last_insert_rowid(db.get());
  }
  
  template<typename T>
  void insertMessage(const Message<T>& m) {
    // std::cout << __PRETTY_FUNCTION__ << '\n';
    auto messageID = insertMessageHeader(m);
    for (const auto& rowset : m.rowsets)
      insertRowset(rowset, messageID);
    for (const auto& uploadKey : m.uploadKeys) {
      auto uploadKeyID = insertUploadKey(uploadKey);
      associateUploadKey(uploadKeyID, messageID);
    }
  }
  
  void handleMessageString(const std::string& s) {
    // std::cout << __PRETTY_FUNCTION__ << '\n';
    rapidjson::Document v;
    // v.Parse<rapidjson::kParseStopWhenDoneFlag>(s.c_str());
    v.Parse<0>(s.c_str());
    
    // std::cout << v.GetType() << '\n';
    auto resultType = rapidjson::getMember<std::string>(v, "resultType");
    if (resultType == "history") {
      auto m = rapidjson::get<Message<History>>(v);
      insertMessage<History>(m);
    } else if (resultType == "orders") {
      auto m = rapidjson::get<Message<Orders>>(v);
      insertMessage<Orders>(m);
    } else
      throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + ": ResultType is not \"history\"");
  }
  

  bool handleMessageStream(rapidjson::FileReadStream& f) {
    // std::cout << __PRETTY_FUNCTION__ << '\n';
    rapidjson::Document v;
    v.ParseStream<rapidjson::kParseStopWhenDoneFlag>(f);

    if (v.HasParseError()) {
      std::cout << "Parse error at " << v.GetErrorOffset() << ": " << rapidjson::GetParseError_En(v.GetParseError()) << '\n';
      return false;
    }
  
    try {
    auto resultType = rapidjson::getMember<std::string>(v, "resultType");
    if (resultType == "history") {
      auto m = rapidjson::get<Message<History>>(v);
      insertMessage<History>(m);
    } else if (resultType == "orders") {
      auto m = rapidjson::get<Message<Orders>>(v);
      insertMessage<Orders>(m);
    } else
      throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + ": ResultType is not \"history\"");
    } catch (const std::exception& e) {
      std::cout << "failed to handle message: " << e.what() << '\n';
      printJSON(backlog, v, 0);
      backlog.flush();
    }
    return true;
  }

};

template<>
struct emdr_db::insert_impl<Orders> {
  static void insert(const sqlite::stmtPtr& stmt, const Orders& orders, std::size_t headerID) {
    // std::cout << __PRETTY_FUNCTION__ << '\n';
    using namespace sqlite;
    sqlite::bind(stmt, 1, orders.orderID);
    sqlite::bind(stmt, 2, headerID);
    sqlite::bind(stmt, 3, orders.price);
    sqlite::bind(stmt, 4, orders.volRemaining);
    sqlite::bind(stmt, 5, orders.range);
    sqlite::bind(stmt, 6, orders.volumeEntered);
    sqlite::bind(stmt, 7, orders.minVolume);
    sqlite::bind(stmt, 8, orders.bid);
    sqlite::bind(stmt, 9, orders.issueDate.time_since_epoch().count());
    sqlite::bind(stmt, 10, orders.duration.count());
    sqlite::bind(stmt, 11, orders.stationID);
    sqlite::bind(stmt, 12, orders.solarSystemID);
    step(stmt);
    sqlite::Resetter resetter(stmt);
  }
};

template<>
const sqlite::stmtPtr& emdr_db::getStmt<Orders>() {
  return insertOrdersStmt;
}

template<>
const sqlite::stmtPtr& emdr_db::getStmt<History>() {
  return insertHistoryStmt;
}

template<>
struct emdr_db::insert_impl<History> {
  static void insert(const sqlite::stmtPtr& stmt, const History& history, std::size_t headerID) {
    // std::cout << __PRETTY_FUNCTION__ << '\n';
    using namespace sqlite;
    sqlite::bind(stmt, 1, headerID);
    sqlite::bind(stmt, 2, history.date.time_since_epoch().count());
    sqlite::bind(stmt, 3, history.orders);
    sqlite::bind(stmt, 4, history.low);
    sqlite::bind(stmt, 5, history.high);
    sqlite::bind(stmt, 6, history.average);
    sqlite::bind(stmt, 7, history.quantity);
    step(stmt);
    sqlite::Resetter resetter(stmt);
  }
};

bool sigint = false;
void sigint_handler(int signal) {
  sigint = true;
}

struct order {
  double price;
  std::uint64_t station;
};
struct itemInfo {
  order maxBuy;
  order minSell;
};

std::unordered_map<std::uint64_t, itemInfo> maxPriceDiff(const Message<Orders>& m) {
    // handle orders
    using entries_t = std::unordered_map<std::uint64_t, itemInfo>;
    static entries_t entries;
    for (const auto& rs : m.rowsets) {
      auto& entry = [&] () -> itemInfo& {
        auto entry = entries.find(rs.typeID);
        if (entry == entries.end())
          entry = entries.insert({rs.typeID, {{-std::numeric_limits<double>::infinity()}, {std::numeric_limits<double>::infinity()}}}).first;
        return entry->second;
      }();
      for (const auto& o : rs.x) {
        if (o.bid && o.price > entry.maxBuy.price) {
          entry.maxBuy.price = o.price;
          entry.maxBuy.station = o.stationID;
        } else if (!o.bid && o.price < entry.minSell.price) {
          entry.minSell.price = o.price;
          entry.minSell.station = o.stationID;
        }
      }
    }

    std::vector<entries_t::iterator> es(entries.size());
    std::iota(es.begin(), es.end(), entries.begin());
    std::sort(es.begin(), es.end(), [] (const entries_t::iterator& a, const entries_t::iterator& b) { return a->second.maxBuy.price - a->second.minSell.price < b->second.maxBuy.price - b->second.minSell.price; });
    for(const auto& e : es) {
      auto gain = e->second.maxBuy.price - e->second.minSell.price;
      if (gain > 500000 && e->second.minSell.price < 12e6)
        std::cout << e->first << ": Buy for " << e->second.minSell.price << " at " << e->second.minSell.station << " -> Sell for " << e->second.maxBuy.price << " at " << e->second.maxBuy.station << " (Gain: " << gain << ")\n";
    }
    std::fill_n(std::ostream_iterator<char>(std::cout), 80, '=');
    std::cout << '\n';
    return entries;
}

namespace maxIskHaul {
  struct order {
    std::size_t quantity;
    double price;
  };
  
  struct orderList {
    std::unordered_map<std::size_t,order> orders;
  };

  struct stationData {
    std::unordered_map<std::size_t,orderList> buyOrders;
    std::unordered_map<std::size_t,orderList> sellOrders;
  };

  std::unordered_map<std::size_t,stationData> stations;
  std::unordered_map<std::size_t, std::set<std::size_t>> freshData;
  std::mutex stationMutex;

void updateMaxIskHaulData(sqlite::dbPtr& eve, sqlite::stmtPtr& insertStmt, const Message<Orders>& m) {
  for (const auto& rs : m.rowsets) {
    auto itemId = rs.typeID;
    std::set<std::size_t> stationIDs;
    
    for (const auto& o : rs.x) {
      stationIDs.insert(o.stationID);
    }

    if (stationIDs.empty())
      continue;

    auto stationIt = stationIDs.begin();
    auto lastUpdateStmt = sqlite::prepare(eve, "select lastupdate from last_update where regionid = " + std::to_string(rs.regionID) + ";");
    
    auto rc = sqlite::step(lastUpdateStmt);
    if (rc == SQLITE_ROW) {
      auto lastUpdate = sqlite::column<std::int64_t>(lastUpdateStmt, 0);
      //std::cout << "lastUpdate = " << lastUpdate << '\n';
      if (lastUpdate >= rs.generatedAt.time_since_epoch().count()) {
        //std::cout << "too old\n";
        continue;
      }
      //std::cout << "new data\n";
    } else if (rc == SQLITE_DONE) {
      //std::cout << "not seen before\n";
    } else
      throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + "/sqlite::step(lastUpdate):  " + sqlite3_errstr(rc) + " (" + std::to_string(rc) + ")");

    std::stringstream stationsSql;
    stationsSql << "(" << *stationIt++;
    for (; stationIt != stationIDs.end(); ++stationIt) {
      auto& station = stations[*stationIt];
      station.buyOrders[itemId].orders.clear();
      station.sellOrders[itemId].orders.clear();
      freshData[*stationIt].insert(itemId);
      stationsSql << ", " << *stationIt;
    }
    stationsSql << ")";
    std::stringstream deleteSql;
    //deleteSql << "delete from orders where stationID in " << stationsSql.rdbuf() << " and typeID = " << itemId << ";";
    deleteSql << "delete from orders where (stationID in (select stationID from staStations where regionID = " << rs.regionID << ") or stationID in (select stationID from regions where regionID = " << rs.regionID << ") or stationID in " << stationsSql.str() << ") and typeID = " << itemId << ";";
    auto deleteStmt = sqlite::prepare(eve, deleteSql.str());
    rc = sqlite::step(deleteStmt);

    auto deletedRows = sqlite3_changes(eve.get());
    //std::cout << "deletedRows: " << deletedRows << '\n';

    for (const auto& o : rs.x) {
      auto stationId = o.stationID;
      auto& station = stations[stationId];
      auto& orders = o.bid ? station.buyOrders[itemId] : station.sellOrders[itemId];
      auto& order = orders.orders[o.orderID];
      order.quantity = o.volRemaining;
      order.price = o.price;
      sqlite::Resetter reset(insertStmt);

      sqlite::bind(insertStmt, 1, o.orderID);
      sqlite::bind(insertStmt, 2, itemId);
      sqlite::bind(insertStmt, 3, o.bid);
      sqlite::bind(insertStmt, 4, o.price);
      sqlite::bind(insertStmt, 5, o.volRemaining);
      sqlite::bind(insertStmt, 6, o.stationID);
      sqlite::bind(insertStmt, 7, (o.issueDate + o.duration).time_since_epoch().count());
      //sqlite::bind(insertStmt, 8, rs.generatedAt.time_since_epoch().count());
      sqlite::step(insertStmt);
    }
    auto insertLastUpdateStmt = sqlite::prepare(eve, "insert or replace into last_update values (" + std::to_string(rs.regionID) + ", " + std::to_string(rs.typeID) + ", " + std::to_string(rs.generatedAt.time_since_epoch().count()) + ");");
    sqlite::step(insertLastUpdateStmt);
  }
}

void helper(std::size_t stationA_, const stationData& stationA, const std::set<std::size_t>& itemsA, std::size_t stationB_, const stationData& stationB, const std::set<std::size_t>& itemsB, const std::unordered_map<std::size_t,double>& volume, const std::unordered_map<std::size_t,std::string>& name) {
  std::vector<std::size_t> intersection;
  intersection.reserve(std::min(itemsA.size(), itemsB.size()));
  std::set_intersection(itemsA.begin(), itemsA.end(), itemsB.begin(), itemsB.end(), std::back_inserter(intersection));
  if (intersection.empty()) {
    return;
  }

  bool profitable = false;
  for (auto itemID : intersection) {
    const auto& buyOrders = stationB.buyOrders.at(itemID).orders;
    auto maxBuy = *std::max_element(buyOrders.begin(), buyOrders.end(), [] (std::unordered_map<std::size_t,order>::const_reference a, std::unordered_map<std::size_t,order>::const_reference b) {
        return a.second.price > b.second.price;
        });
    const auto& sellOrders = stationA.sellOrders.at(itemID).orders;
    auto minSell = *std::min_element(sellOrders.begin(), sellOrders.end(), [] (std::unordered_map<std::size_t,order>::const_reference a, std::unordered_map<std::size_t,order>::const_reference b) {
        return a.second.price < b.second.price;
        });
    if (minSell.second.price < maxBuy.second.price) {
      profitable = true;
      break;
    }
  }

  if (!profitable)
    return;
  profitable = false;

  auto constraintCount = 2u + intersection.size();
  auto variableCount = 0u;
  for (auto itemID : intersection) {
    auto orderCount = stationA.sellOrders.at(itemID).orders.size() + stationB.buyOrders.at(itemID).orders.size();
    variableCount += 2*orderCount;
    constraintCount += 2*orderCount;
  }

  std::vector<std::vector<double>> A(constraintCount, std::vector<double>(variableCount, 0.0));
  std::vector<double> b(constraintCount, 0.0);
  std::vector<double> c(variableCount, 0.0);

  auto v = 0u;
  auto constr = 0u;
  auto currentTypeConstraint = constraintCount - intersection.size() - 2;
  auto capacityConstraint = constraintCount - 2;
  b[capacityConstraint] = 3900;
  auto wealthConstraint = constraintCount - 1;
  b[wealthConstraint] = 75e6;
  enum class OrderType {
    BUY, SELL
  };
  struct variableInfo {
    std::size_t itemID;
    std::size_t orderID;
    OrderType type;
  };
  std::vector<variableInfo> variableMap;
  variableMap.reserve(variableCount);
  for (auto itemID : intersection) {
    auto vol = volume.at(itemID);
    if (profitable) {
      std::cout << "itemID " << itemID << '\n';
      std::cout << "sell orders:\n";
    }
    for (const auto& order : stationA.sellOrders.at(itemID).orders) {
      variableMap.push_back({ itemID, order.first, OrderType::BUY });
      A[constr][v] = 1;
      b[constr] = order.second.quantity;
      ++constr;
      if (profitable)
        std::cout << "x" << v << " <= " << order.second.quantity << '\n';

      A[constr][v] = -1;
      b[constr] = 0;
      ++constr;

      A[currentTypeConstraint][v] = -1;
      A[capacityConstraint][v] = vol;
      A[wealthConstraint][v] = order.second.price;
      c[v] = -order.second.price;
      ++v;
    }

    if (profitable)
      std::cout << "buy orders:\n";
    for (const auto& order : stationB.buyOrders.at(itemID).orders) {
      variableMap.push_back({ itemID, order.first, OrderType::SELL });
      A[constr][v] = 1;
      b[constr] = order.second.quantity;
      ++constr;
      if (profitable)
        std::cout << "x" << v << " <= " << order.second.quantity << '\n';

      A[constr][v] = -1;
      b[constr] = 0;
      ++constr;

      A[currentTypeConstraint][v] = 1;
      c[v] = order.second.price;
      ++v;
    }

    ++currentTypeConstraint;
  }

  if (profitable) {
    std::cout << "wealth constraint: \n";
    for (auto v = 0u; v < variableCount; ++v)
      if (A[wealthConstraint][v] > 0)
        std::cout << A[wealthConstraint][v] << "*x" << v << " + ";
    std::cout << "<= " << b[wealthConstraint] << '\n';

    std::cout << "capacity constraint: \n";
    for (auto v = 0u; v < variableCount; ++v)
      if (A[capacityConstraint][v] > 0)
        std::cout << A[capacityConstraint][v] << "*x" << v << " + ";
    std::cout << "<= " << b[capacityConstraint] << '\n';
  }

  auto start = std::chrono::system_clock::now();
  try {
    LinearOptimizer simplex(c, A, b);
    static double maxValue = 0;
    if (profitable || simplex.value() > 0) {
      maxValue = std::max(maxValue, simplex.value());
      std::cout << "maxValue: " << maxValue << '\n';
      std::cout << stationA_ << " -> " << stationB_ << '\n';
      std::cout << "value: " << simplex.value() << "ISK\n";
      // TODO: Print required volume and ISK, use map information (station names, security status, jumps)
      //       write options sorted by value to file and replace file on update (do not append)
      //       remove capacity constraint if origin and destination are the same
      const auto& solution = simplex.getResult();
      std::cout << "solution: \n";
      for (auto v = 0u; v < variableCount; ++v) {
        const auto& info = variableMap[v];
        if (solution[v] > 0)
          std::cout << (info.type == OrderType::BUY ? "Buy " : "Sell ") << solution[v] << " of \"" <<  name.at(info.itemID) << "\" at " << (info.type == OrderType::BUY ? stationA.sellOrders.at(info.itemID).orders.at(info.orderID).price : stationB.buyOrders.at(info.itemID).orders.at(info.orderID).price) << "ISK\n";
      }
    }
  } catch (const std::exception& e) {
    std::cout << "simplex failed: " << e.what() << '\n';
  }

  if (profitable)
    std::cout << constraintCount << ", " << variableCount << ": " << std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::system_clock::now() - start).count() << "s\n";
}

void maxIskHaul(const std::unordered_map<std::size_t,stationData>& stations_, const std::unordered_map<std::size_t,double>& volume, const std::unordered_map<std::size_t,std::string>& name) {
  auto first = [] (std::unordered_map<std::size_t,orderList>::const_reference i) { return i.first; };
  for (const auto& stationA_ : stations_) {
    const auto& stationA = stationA_.second;
    std::set<std::size_t> itemsA_full;
    for (const auto& order : stationA.sellOrders)
      if (!order.second.orders.empty())
        itemsA_full.insert(order.first);
    std::set<std::size_t> itemsA;
    //itemsA.reserve(stationA_.second.size());
    //std::set_intersection(stationA_.second.begin(), stationA_.second.end(), itemsA_full.begin(), itemsA_full.end(), std::inserter(itemsA, itemsA.end()));
    for (const auto& stationB_ : stations_) {
      const auto& stationB = stationB_.second;
      std::set<std::size_t> itemsB;
      //itemsB.reserve(stationB.buyOrders.size());
      for (const auto& order : stationB.buyOrders)
        if (!order.second.orders.empty())
          itemsB.insert(order.first);
      helper(stationA_.first, stationA, itemsA_full, stationB_.first, stationB, itemsB, volume, name);
    }
  }
}

}

void handleMessageString(const std::string& s, boost::circular_buffer<Message<Orders>>& queue, std::mutex& mutex_) {
  // std::cout << __PRETTY_FUNCTION__ << '\n';
  rapidjson::Document v;
  // v.Parse<rapidjson::kParseStopWhenDoneFlag>(s.c_str());
  v.Parse<0>(s.c_str());

  // std::cout << v.GetType() << '\n';
  auto resultType = rapidjson::getMember<std::string>(v, "resultType");
  if (resultType == "history") {
    auto m = rapidjson::get<Message<History>>(v);
    // handle history
  } else if (resultType == "orders") {
    auto m = rapidjson::get<Message<Orders>>(v);
    //maxPriceDiff(m);
    //maxIskHaul::updateMaxIskHaulData(m);
    std::lock_guard<std::mutex> lock(mutex_);
    queue.push_back(m);
  } else
    throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + ": ResultType is \"" + resultType + "\"");
}

int main(int argc, char** argv) {
//  //std::ifstream file("emdr.data.3");
//  //std::stringstream msg;
//  //file >> msg.rdbuf();
//  if (argc < 2) {
//    std::cout << "too few arguments\n";
//    return 0;
//  }
//
  std::signal(SIGINT, &sigint_handler);
//
//  FILE * pFile = fopen (argv[1],"r");
//  if (!pFile) {
//    fclose (pFile);
//    std::cout << "fopen failed\n";
//    return 0;
//  }
//  std::string buffer(4194304, ' ');
//  rapidjson::FileReadStream input(pFile, &buffer[0], buffer.size());
//  emdr_db db("emdr.db");
//
//  //Transaction asdf(db.beginTransactionStmt, db.endTransactionStmt);
//  // auto rc = sqlite::step(db.databaseListStmt);
//  // while (rc == SQLITE_ROW) {
//  //   auto id = sqlite::column<std::int64_t>(db.databaseListStmt, 0);
//  //   auto name = sqlite::column<const std::uint8_t*>(db.databaseListStmt, 1);
//  //   auto file = sqlite::column<const std::uint8_t*>(db.databaseListStmt, 2);
//
//  //   std::cout << id << " | " << name << " | " << file << '\n';
//  //   rc = sqlite::step(db.databaseListStmt);
//  // }
//
//  // db.handleMessageString(msg.str());
//
//  sqlite::step(db.beginTransactionStmt);
//  sqlite::reset(db.beginTransactionStmt);
//
//  bool notDone = true;
  using clock = std::chrono::system_clock;
//  auto lastTime = clock::now();
//  auto startTime = lastTime;
//  auto lastPos = input.Tell();
//  auto count = 0;
//  auto lastCount = count;
//  while (notDone && !sigint) {
//    notDone = db.handleMessageStream(input);
//    count++;
//    auto pos = input.Tell();;
//    auto time = clock::now();
//    auto tdiff = time - lastTime;
//    auto pdiff = pos - lastPos;
//    auto cdiff = count - lastCount;
//    if (tdiff > std::chrono::seconds{1}) {
//      //sqlite::step(db.endTransactionStmt);
//      //sqlite::reset(db.endTransactionStmt);
//      // int nLog, nCkpt;
//      // auto rc = sqlite3_wal_checkpoint_v2(db.db.get(), nullptr, SQLITE_CHECKPOINT_FULL, &nLog, &nCkpt);
//      // std::cout << "checkpoint: nLog = " << nLog << ", nCkpt = " << nCkpt << ", rc = " << rc << " (" << sqlite3_errstr(rc) << ")\n";
//      //sqlite::step(db.beginTransactionStmt);
//      //sqlite::reset(db.beginTransactionStmt);
//      auto totalTime = time - startTime;
//      std::fill_n(std::ostream_iterator<char>(std::cout), 80, '=');
//      std::cout << '\n';
//      std::cout << "Done: " << (double(pos) / double(1 << 20)) << " MB\n";
//      std::cout << "Speed: " << (double(pdiff) / double(1 << 20) / std::chrono::duration_cast<std::chrono::duration<double>>(tdiff).count()) << " MB/s\n";;
//      std::cout << "Average speed: " << (double(pos) / double(1 << 20) / std::chrono::duration_cast<std::chrono::duration<double>>(totalTime).count()) << " MB/s\n";
//      std::cout << '\n';
//      std::cout << "Message count: " << count << '\n';
//      std::cout << "Speed: " << (double(cdiff) / std::chrono::duration_cast<std::chrono::duration<double>>(tdiff).count()) << " / s\n";;
//      std::cout << "Average speed: " << (double(count) / std::chrono::duration_cast<std::chrono::duration<double>>(totalTime).count()) << " / s\n";
//      lastPos = pos;
//      lastTime = time;
//      lastCount = count;
//    }
//    
//    // if (pos > (1 << 30))
//    //   break;
//  }
//  sqlite::step(db.endTransactionStmt);
//  sqlite::reset(db.endTransactionStmt);
//  
//  return 0;

  auto eve = sqlite::connectDB("eve.sqlite");
  auto itemCountStmt = sqlite::prepare(eve, "select count(*) from invTypes;");
  auto rc = sqlite::step(itemCountStmt);
  assert(rc == SQLITE_ROW);
  auto itemCount = sqlite::column<std::int64_t>(itemCountStmt, 0);
  auto items = sqlite::prepare(eve, "select typeID, typeName, volume from invTypes;");
  std::unordered_map<std::size_t,std::string> itemName(itemCount);
  std::unordered_map<std::size_t,double> itemVolume(itemCount);
  while(sqlite::step(items) == SQLITE_ROW) {
    auto id = sqlite::column<std::int64_t>(items, 0);
    auto name = std::string((const char*)sqlite::column<const std::uint8_t*>(items, 1));
    auto volume = sqlite::column<double>(items, 2);
    itemName.insert({id, name});
    itemVolume.insert({id, volume});
  }
  //std::thread maxIskHaulThread([&itemVolume,&itemName] {
  //  std::unordered_map<std::size_t,maxIskHaul::stationData> stations_;
  //  std::unordered_map<std::size_t,std::set<std::size_t>> freshData_;
  //  while(!sigint) {
  //    std::this_thread::sleep_for(std::chrono::seconds{1});
  //    {
  //      std::lock_guard<std::mutex> lock(maxIskHaul::stationMutex);
  //      stations_ = maxIskHaul::stations;
  //      freshData_ = maxIskHaul::freshData;
  //      maxIskHaul::freshData.clear();
  //    }
  //    maxIskHaul::maxIskHaul(stations_, freshData_, itemVolume, itemName);
  //  }
  //});

  zmq::context_t context(1);
  zmq::socket_t socket(context, ZMQ_SUB);
  socket.connect("tcp://relay-eu-germany-1.eve-emdr.com:8050");
  socket.setsockopt(ZMQ_SUBSCRIBE, nullptr, 0);


  namespace io = boost::iostreams;
  rapidjson::Document d;
  boost::circular_buffer<Message<Orders>> messageBuffer(1000);
  std::thread maxIskHaulThread([&messageBuffer] {
    auto eve = [] {
      auto db = sqlite::connectDB("marketData.sqlite");
      for (const auto& pragma : { "pragma cache_size=2000000;", "pragma synchronous=OFF;", "pragma temp_store=Memory;", "pragma journal_mode=MEMORY;", "PRAGMA wal_autocheckpoint=1000;", "attach \"eve.sqlite\" as eve;" }) {
        int rc;
        do {
          sqlite::step(sqlite::prepare(db, pragma));
        } while(rc == SQLITE_ROW);
      }
      auto rc = sqlite3_extended_result_codes(db.get(), 1);
      if (rc)
        std::cout << __PRETTY_FUNCTION__ << ":  " << sqlite3_errstr(rc) << " (" << rc << ")\n";
      return db;
    }();

    auto beginTransaction = sqlite::prepare(eve, "begin;");
    auto endTransaction = sqlite::prepare(eve, "end;");
    auto rollbackTransaction = sqlite::prepare(eve, "rollback;");
    auto insertStmt = sqlite::prepare(eve, "insert or replace into orders values (?, ?, ?, ?, ?, ?, ?);");

    boost::circular_buffer<Message<Orders>> localMessageBuffer(1000);
    while(!sigint) {
      try {
        std::this_thread::sleep_for(std::chrono::seconds{1});
        //std::cout << "updating\n";
        auto time = clock::now();

        {
          std::lock_guard<std::mutex> lock(maxIskHaul::stationMutex);
          localMessageBuffer.insert(localMessageBuffer.end(), messageBuffer.begin(), messageBuffer.end());
          messageBuffer.clear();
        }
        auto count = localMessageBuffer.size();
        //std::cout << "queue: " << count << '\n';

        {
          sqlite::Transaction transaction(beginTransaction, endTransaction, rollbackTransaction);
          // while(!localMessageBuffer.empty()) {
          //   const auto& m = localMessageBuffer.front();
          //   maxIskHaul::updateMaxIskHaulData(eve, insertStmt, m);
          //   localMessageBuffer.pop_front();
          // }
          for (const auto& m : localMessageBuffer)
            maxIskHaul::updateMaxIskHaulData(eve, insertStmt, m);
          auto deleteStmt = sqlite::prepare(eve, "delete from orders where availableUntil < " + std::to_string(time.time_since_epoch().count()) + ";");

          transaction.commit();
          localMessageBuffer.clear();
        }
        auto diff = clock::now() - time;
        std::cout << "[DB UPDATE] Count: " << count << ", Speed: " << (count / std::chrono::duration_cast<std::chrono::duration<double>>(diff).count()) << "/s\n";
      } catch (const std::exception& e) {
        std::cout << "[ERROR] " << e.what() << '\n';
      } catch (...) {
        std::cout << "[ERROR] unknown exception type\n";
      }
    }
  });
  

  auto lastTime = clock::now();
  auto startTime = lastTime;
  auto count = 0;
  auto lastCount = count;
  clock::duration sqliteTime(0);
  auto lastSqliteTime = sqliteTime;
  while(!sigint) {
    try {
      //        std::cout << "Waiting ...\n";
      zmq::message_t request;
      try {
        socket.recv(&request);
      } catch (const zmq::error_t& e) {
        if (e.num() == EINTR)
          sigint = true;
        else
          throw;
      }
      //        std::cout << "Received " << request.size() << " Bytes\n";
      io::filtering_ostream in;

      in.push(io::zlib_decompressor());
      std::stringstream out;
      in.push(out);
      in.write(static_cast<const char*>(request.data()), request.size());
      in.flush();
      auto timeA = clock::now();
      //handleMessageString(out.str(), itemVolume, itemName);
      handleMessageString(out.str(), messageBuffer, maxIskHaul::stationMutex);
      sqliteTime += clock::now() - timeA;
      count++;
      auto time = clock::now();
      auto tdiff = time - lastTime;
      if (tdiff > std::chrono::seconds{1}) {
        //sqlite::step(db.endTransactionStmt);
        //sqlite::reset(db.endTransactionStmt);
        // int nLog, nCkpt;
        // auto rc = sqlite3_wal_checkpoint_v2(db.db.get(), nullptr, SQLITE_CHECKPOINT_FULL, &nLog, &nCkpt);
        // std::cout << "checkpoint: nLog = " << nLog << ", nCkpt = " << nCkpt << ", rc = " << rc << " (" << sqlite3_errstr(rc) << ")\n";
        //sqlite::step(db.beginTransactionStmt);
        //sqlite::reset(db.beginTransactionStmt);
        auto totalTime = time - startTime;
        auto sdiff = sqliteTime - lastSqliteTime;
        auto sqlitePct = double(sqliteTime.count()) / double(totalTime.count());
        auto lastSqlitePct = double(sdiff.count()) / double(tdiff.count());
        auto cdiff = count - lastCount;
        std::fill_n(std::ostream_iterator<char>(std::cout), 80, '=');
        std::cout << '\n';
        std::cout << "Queue size: " << messageBuffer.size() << '\n';
        std::cout << "Message count: " << count << '\n';
        std::cout << "Total Message Handling Percentage: " << (100 * sqlitePct) << "%\n";
        std::cout << "Last Message Handling Percentage: " << (100 * lastSqlitePct) << "%\n";
        std::cout << "Speed: " << (double(cdiff) / std::chrono::duration_cast<std::chrono::duration<double>>(tdiff).count()) << " / s\n";;
        std::cout << "Average speed: " << (double(count) / std::chrono::duration_cast<std::chrono::duration<double>>(totalTime).count()) << " / s\n";
        lastTime = time;
        lastCount = count;
        lastSqliteTime = sqliteTime;
      }

      //        const auto& str = out.str();
      //        std::cout << str << '\n';
      //        std::cout << "Parsing " << str.size() << " Bytes ... ";
      //        d.Parse<0>(out.str().c_str());
      //        if (d.GetParseError())
      //            std::cout << d.GetParseError();
      //        std::cout << '\n' << "Printing ...\n";
      //        printJSON(std::cout, d, 0);
      //        rapidjson::StringBuffer buffer;
      //        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      //        d.Accept(writer);
      //
      //        std::cout << buffer.GetString() << '\n';


      //        std::cout << "\n======================================================================\n";
      std::cout.flush();
    } catch (const std::exception& e) {
      std::cout << "[ERROR] " << e.what() << '\n';
    } catch (...) {
      std::cout << "[ERROR] Unknown exception type\n";
    }
  }
  if (sigint)
    std::cout << "shutting down\n";
  maxIskHaulThread.join();

  return 0;
}

int main2(int argc, char** args) {
  auto eve = [] {
    auto db = sqlite::connectDB("marketData.sqlite");
    for (const auto& pragma : { "pragma cache_size=2000000;", "pragma synchronous=OFF;", "pragma temp_store=Memory;", "pragma journal_mode=MEMORY;", "PRAGMA wal_autocheckpoint=1000;", "attach \"eve.sqlite\" as eve;" }) {
      int rc;
      do {
        sqlite::step(sqlite::prepare(db, pragma));
      } while(rc == SQLITE_ROW);
    }
    auto rc = sqlite3_extended_result_codes(db.get(), 1);
    if (rc)
      std::cout << __PRETTY_FUNCTION__ << ":  " << sqlite3_errstr(rc) << " (" << rc << ")\n";
    return db;
  }();

  auto itemCountStmt = sqlite::prepare(eve, "select count(*) from invTypes;");
  auto rc = sqlite::step(itemCountStmt);
  assert(rc == SQLITE_ROW);
  auto itemCount = sqlite::column<std::int64_t>(itemCountStmt, 0);
  auto items = sqlite::prepare(eve, "select typeID, typeName, volume from invTypes;");
  std::unordered_map<std::size_t,std::string> itemName(itemCount);
  std::unordered_map<std::size_t,double> itemVolume(itemCount);
  while(sqlite::step(items) == SQLITE_ROW) {
    auto id = sqlite::column<std::int64_t>(items, 0);
    auto name = std::string((const char*)sqlite::column<const std::uint8_t*>(items, 1));
    auto volume = sqlite::column<double>(items, 2);
    itemName.insert({id, name});
    itemVolume.insert({id, volume});
  }
  
  auto ordersStmt = sqlite::prepare(eve, "select * from orders;");
  while(sqlite::step(ordersStmt) == SQLITE_ROW) {
    auto orderID = sqlite::column<std::int64_t>(ordersStmt, 0);
    auto typeID = sqlite::column<std::int64_t>(ordersStmt, 1);
    bool bid = sqlite::column<std::int64_t>(ordersStmt, 2);
    auto price = sqlite::column<double>(ordersStmt, 3);
    auto volRemaining = sqlite::column<std::int64_t>(ordersStmt, 4);
    auto stationID = sqlite::column<std::int64_t>(ordersStmt, 5);
    auto availableUntil = sqlite::column<std::int64_t>(ordersStmt, 6);
    auto& station = maxIskHaul::stations[stationID];
    auto& orders = bid ? station.buyOrders : station.sellOrders;
    auto& order = orders[typeID].orders[orderID];
    order.quantity = volRemaining;
    order.price = price;
  }
  maxIskHaul::maxIskHaul(maxIskHaul::stations, itemVolume, itemName);

  return 0;
}

