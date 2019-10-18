#pragma once

#include "libs/industry_resources/blueprint_efficiency.h"
#include "libs/industry_resources/blueprint_with_efficiency.h"
#include "libs/industry_resources/decryptor_property.h"
#include "libs/industry_resources/resource_base.h"
#include <cstddef>
#include <unordered_map>
#include <vector>

class BlueprintWithEfficiency;
struct IndustryLimits;
struct Names;
struct Assets;
struct Types;

struct NormalResources {
    NormalResources(const Names& names, const Assets& assets);
    const Resource_Base::Ptr& getResource(const std::size_t id);

  private:
    std::unordered_map<std::size_t, Resource_Base::Ptr> resources_;
    const Names& names_;
    const Assets& assets_;
};

struct BlueprintOriginals {
    BlueprintOriginals(const Names& names, double timeFrame);
    const Resource_Base::Ptr& getBPO(const std::size_t id);

  private:
    std::unordered_map<std::size_t, Resource_Base::Ptr> bpoTimes_;
    const Names& names_;
    double timeFrame_;
};

struct BlueprintCopies {
    BlueprintCopies(const Names& names, const Assets& assets, const Types& types);
    const Resource_Base::Ptr& getBPC(const BlueprintWithEfficiency& bpwe);

  private:
    std::unordered_map<BlueprintWithEfficiency, Resource_Base::Ptr> bpcs_;
    const Names& names_;
    const Assets& assets_;
    const Types& types_;
};

struct Stuff : private NormalResources, private BlueprintOriginals, private BlueprintCopies {
    Stuff(const Names& names, const Assets& assets, const Types& types, const IndustryLimits& industry);

    using BlueprintCopies::getBPC;
    using BlueprintOriginals::getBPO;
    using NormalResources::getResource;

    static const std::vector<BlueprintEfficiency> bpes;
    static const std::vector<DecryptorProperty> decryptors;

    const Resource_Base::Ptr labTime;
    const Resource_Base::Ptr productionLineTime;

  private:
    static const std::vector<DecryptorProperty> t1decryptors;
};
