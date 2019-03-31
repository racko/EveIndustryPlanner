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

    size_t countAsset(size_t typeId) const;

    size_t countPlanetaryAsset(size_t typeId) const;

    size_t runs(const BlueprintWithEfficiency& bpwe) const;

    // for t1 bpcs we ignore efficiencies because we would have to deal with all 100 research level combinations
    size_t runs(size_t typeId) const;
private:
    std::unordered_map<size_t, size_t> assets;
    std::unordered_map<size_t, size_t> planetary_assets;
    std::unordered_map<BlueprintWithEfficiency, size_t> ownedBPCs;

};
