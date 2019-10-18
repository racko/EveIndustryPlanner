#pragma once

#include "blueprint_with_efficiency.h"

#include <unordered_map>
#include <unordered_set>

struct Names;

struct OwnedBPCs {
    void loadOwnedBPCsFromXml(const Names& names);

    std::size_t runs(const BlueprintWithEfficiency& bpwe) const;

    // for t1 bpcs we ignore efficiencies because we would have to deal with all 100 research level combinations
    std::size_t runs(std::size_t typeId) const;

  private:
    std::unordered_map<BlueprintWithEfficiency, std::size_t> ownedBPCs_;
};

struct PlanetaryAssets {
    // NIY: currently always returns 0
    std::size_t countPlanetaryAsset(std::size_t typeId) const;

  private:
    std::unordered_map<std::size_t, std::size_t> planetary_assets_;
};

struct NormalAssets {
    void addAssetsFromTsv(const char* filename, const Names& names);

    void loadAssetsFromTsv(const Names& names);

    std::size_t countAsset(std::size_t typeId) const;

  private:
    std::unordered_map<std::size_t, std::size_t> assets_;

    // not called because time between api calls is too long
    void loadAssetsFromXml(const Names& names);
};

struct Assets : private NormalAssets, private OwnedBPCs, private PlanetaryAssets {
    using NormalAssets::addAssetsFromTsv;
    using NormalAssets::countAsset;
    using NormalAssets::loadAssetsFromTsv;

    using OwnedBPCs::loadOwnedBPCsFromXml;
    using OwnedBPCs::runs;

    using PlanetaryAssets::countPlanetaryAsset;
};
