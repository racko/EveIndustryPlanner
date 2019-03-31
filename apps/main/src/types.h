#pragma once

#include "tech_level_fwd.h"
#include "type_id.h"
#include "yaml_fwd.h"

#include <cstddef>
#include <unordered_map>
#include <unordered_set>

struct Groups;

struct Types {
    bool isBP(TypeID t) const;
    bool isT1BP(TypeID t) const;
    bool isT2BP(TypeID t) const;
    bool isT3BP(TypeID t) const;
    bool isRelic(TypeID t) const;
    bool isT1(TypeID t) const;
    bool isT2(TypeID t) const;
    bool isT3(TypeID t) const;
    bool isProduct(TypeID t) const;
    bool isBaseItem(TypeID t) const;

    void registerBlueprint(unsigned blueprintId, const YAML::Node& b, const Groups& groups);

    void reserve(size_t n);

    void isConsistent() const;

    const std::unordered_map<size_t,size_t>& getBlueprints() const { return bpToProduct; }

    TechLevel getBPTechLevel(TypeID t) const;
    TechLevel getProductTechLevel(TypeID t) const;

private:
    bool isBP_(TypeID t) const;
    bool isT1BP_(TypeID t) const;
    bool isT2BP_(TypeID t) const;
    bool isT1_(TypeID t) const;
    bool isT2_(TypeID t) const;
    bool isRelic_(TypeID t) const;
    bool isT3BP_(TypeID t) const;
    bool isT3_(TypeID t) const;
    bool isProduct_(TypeID t) const;
    bool isBaseItem_(TypeID t) const;

    void checkItem(TypeID t) const;

    static bool is_ancient_relic(const YAML::Node& root, const Groups& groups);
    static YAML::Node get_invention_products(const YAML::Node& activities);
    static YAML::Node get_manufacturing_products(const YAML::Node& activities);

    std::unordered_set<size_t> blueprints;
    std::unordered_set<size_t> manufacturingOutputs;
    std::unordered_set<size_t> t2_blueprints;
    std::unordered_set<size_t> t1_blueprints;
    std::unordered_set<size_t> ancient_relics;
    std::unordered_set<size_t> t3_blueprints;
    std::unordered_map<size_t,size_t> t3BPToRelic;
    std::unordered_map<size_t,size_t> t2BPToT1BP;
    std::unordered_map<size_t,size_t> bpToProduct;
    std::unordered_map<size_t,size_t> productToBP;
    std::unordered_multimap<size_t,size_t> t1BPToT2BPs;
    std::unordered_multimap<size_t,size_t> relicToT3BPs;
};
