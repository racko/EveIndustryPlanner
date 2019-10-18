#include "libs/HTTPSRequestHandler/HTTPSRequestHandler.h"
#include "libs/QueryRunner/QueryRunner.h"
#include "profiling.h"
#include "sqlite.h"
#include <sstream>
#include <vector>
#include <yaml-cpp/yaml.h>

struct station {
    station(std::size_t id_, std::string name_, std::size_t region_)
        : id(id_), name(std::move(name_)), region(region_) {}

    std::size_t id;
    std::string name;
    std::size_t region;
};

std::vector<station> loadStations() {
    std::vector<station> stations;
    YAML::Node root;
    auto path = "/cifs/server/Media/Tim/Eve/SDE/sde/bsd/staStations.yaml";
    std::cout << "loading " << path << ". " << std::flush;
    TIME(root = YAML::LoadFile(path););
    std::cout << "Have " << root.size() << " stations\n";
    stations.reserve(root.size());
    std::cout << "building map. " << std::flush;
    TIME(for (const auto& e
              : root) {
        auto stationID = e["stationID"].as<std::uint64_t>();
        const auto& stationName = e["stationName"].as<std::string>();
        auto regionID = e["regionID"].as<std::uint64_t>();
        stations.emplace_back(stationID, std::move(stationName), regionID);
    });
    return stations;
}

void assertStations(const sqlite::dbPtr& db, const std::vector<station>& stations) {
    auto insertStation =
        sqlite::prepare(db, "insert or ignore into stations(stationid, name, regionid) values(?, ?, ?);");

    TIME(for (const auto& s
              : stations) {
        sqlite::bind(insertStation, 1, s.id);
        sqlite::bind(insertStation, 2, s.name);
        sqlite::bind(insertStation, 3, s.region);
        std::cout << "inserting (" << s.id << ", " << s.name << ", " << s.region << ")\n";
        sqlite::step(insertStation);
        sqlite::reset(insertStation);
    });
}

int main() {
    std::cout << std::boolalpha;
    auto db = sqlite::getDB("market_history.db");
    assertStations(db, loadStations());
    return 0;
}
