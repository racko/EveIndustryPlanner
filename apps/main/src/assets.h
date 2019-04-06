#pragma once

#include "blueprint_with_efficiency.h"

#include <unordered_map>
#include <unordered_set>

struct Names;

struct Assets {
    // not called because time between api calls is too long
    void loadAssetsFromXml(const Names& names);

    void addAssetsFromTsv(const char* filename, const Names& names);

    void loadAssetsFromTsv(const Names& names);

    void loadOwnedBPCsFromXml(const Names& names);

    std::size_t countAsset(std::size_t typeId) const;

    std::size_t countPlanetaryAsset(std::size_t typeId) const;

    std::size_t runs(const BlueprintWithEfficiency& bpwe) const;

    // for t1 bpcs we ignore efficiencies because we would have to deal with all 100 research level combinations
    std::size_t runs(std::size_t typeId) const;

  private:
    std::unordered_map<std::size_t, std::size_t> assets;
    std::unordered_map<std::size_t, std::size_t> planetary_assets;
    std::unordered_map<BlueprintWithEfficiency, std::size_t> ownedBPCs;
};
