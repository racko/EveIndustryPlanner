#include "price_reader.h"
#include "HTTPSRequestHandler.h"
#include "QueryRunner.h"
#include "profiling.h"
#include "sqlite.h"
#include <json/json.h>
#include <sstream>
#include <vector>

std::vector<Price> loadPrices() {
    std::vector<Price> prices;
    std::stringstream pricesFile;
    TIME(simpleGet("https://crest-tq.eveonline.com/market/prices/", pricesFile););
    Json::Value root;
    TIME(pricesFile >> root;);
    const auto& items = root["items"];
    std::cout << "Have " << items.size() << " prices\n";
    prices.reserve(items.size());
    std::cout << "building map. " << std::flush;
    TIME(for (const auto& e
              : root["items"]) {
        uint64_t id = e["type"]["id"].asUInt64();
        const auto& name = e["type"]["name"].asString();
        const auto& adj = e["adjustedPrice"];
        const auto& avg = e["averagePrice"];
        // if (!avg.isNull())
        prices.emplace_back(adj.asDouble(), avg.asDouble(), id, std::move(name));
    });
    return prices;
}

std::vector<std::pair<std::size_t, std::string>> loadTypes() {
    std::vector<std::pair<std::size_t, std::string>> types;
    std::stringstream types_stream;
    TIME(simpleGet("https://crest-tq.eveonline.com/market/types/", types_stream););
    do {
        Json::Value root;
        TIME(types_stream >> root;);
        const auto& items = root["items"];
        std::cout << "Have " << items.size() << " types\n";
        types.reserve(types.size() + items.size());
        std::cout << "building map. " << std::flush;
        TIME(for (const auto& e
                  : root["items"]) {
            const auto& type = e["type"];
            uint64_t id = type["id"].asUInt64();
            const auto& name = type["name"].asString();
            types.emplace_back(id, std::move(name));
        });
        types_stream.str("");
        const auto& next = root["next"];
        if (!next.isNull())
            TIME(simpleGet(next["href"].asString(), types_stream););
    } while (!types_stream.str().empty());
    return types;
}

void assertTypes(const sqlite::dbPtr& db, const std::vector<std::pair<std::size_t, std::string>>& prices) {
    auto insertType = sqlite::prepare(db, "insert or ignore into types values(?, ?);");

    TIME(for (const auto& p
              : prices) {
        sqlite::bind(insertType, 1, p.first);
        sqlite::bind(insertType, 2, p.second);
        sqlite::step(insertType);
        sqlite::reset(insertType);
    });
}

int main() {
    std::cout << std::boolalpha;
    auto db = sqlite::getDB("market_history.db");
    assertTypes(db, loadTypes());
    return 0;
}
