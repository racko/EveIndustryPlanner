#pragma once

#include "blueprint_with_efficiency.h"
#include "decryptor_property.h"
#include "resource_base.h"
#include "type_id.h"

#include <cstddef>
#include <unordered_map>
#include <vector>

struct Assets;
struct IndustryLimits;
struct Names;
struct Types;

struct Stuff {
    Stuff(const Names& names, const Assets& assets, const Types& types, const IndustryLimits& industry);

    const Resource_Base::Ptr& getResource(std::size_t id);
    const Resource_Base::Ptr& getBPO(std::size_t id);
    const Resource_Base::Ptr& getBPC(const BlueprintWithEfficiency& bpwe);

    static const std::vector<BlueprintEfficiency> bpes;
    static const std::vector<DecryptorProperty> t1decryptors;
    static const std::vector<DecryptorProperty> decryptors;

    const Resource_Base::Ptr labTime;
    const Resource_Base::Ptr productionLineTime;

  private:
    std::unordered_map<TypeID, Resource_Base::Ptr> resources;
    std::unordered_map<TypeID, Resource_Base::Ptr> bpoTimes;
    std::unordered_map<BlueprintWithEfficiency, Resource_Base::Ptr> bpcs;

    const Names& names_;
    const Assets& assets_;
    const Types& types_;
    const double timeFrame;
};
