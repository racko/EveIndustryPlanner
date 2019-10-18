#include "libs/industry_resources/types/types.h"

#include "groups.h"
#include "tech_level.h"

#include <algorithm>
#include <iostream>
#include <yaml-cpp/yaml.h>

void Types::isConsistent() const {
    //// only blueprints contains t1s without invention. So the following is wrong
    // auto blueprints_ = t1_blueprints;
    // blueprints_.insert(t2_blueprints.begin(), t2_blueprints.end());
    // blueprints_.insert(t3_blueprints.begin(), t3_blueprints.end());
    // auto blueprints_match = blueprints.size() == t1_blueprints.size() + t2_blueprints.size() + t3_blueprints.size()
    // && blueprints == blueprints_;

    // if (!blueprints_match) {
    //    std::cout << "blueprints not in t1, t2 or t3: ";
    //    for (auto p : blueprints) {
    //        if (blueprints_.find(p) == blueprints_.end())
    //            std::cout << p << ' ';
    //    }
    //    std::cout << "\nt1, t2 or t3 not in blueprints: ";
    //    for (auto p : blueprints_) {
    //        if (blueprints.find(p) == blueprints.end())
    //            std::cout << p << ' ';
    //    }
    //    std::cout << '\n';
    //}

    // auto products_match = true;
    // for (auto p : manufacturingOutputs) {
    //    products_match &= int(isT1_(p)) + int(isT2_(p)) + int(isT3_(p)) == 1;
    //}
    // if (!blueprints_match || !products_match)
    //    throw std::runtime_error("types are inconsistent");
}

void Types::reserve(std::size_t n) {
    t2BPToT1BP.reserve(n);
    bpToProduct.reserve(n);
    productToBP.reserve(n);
    t1BPToT2BPs.reserve(n);
}

bool Types::isBP_(TypeID t) const {
    // return isT1BP_(t) || isT2BP_(t) || isT3BP_(t); // cannot use this because we have to base isT1BP_ on isBP_ (see
    // below)
    return blueprints.find(t) != blueprints.end();
}

bool Types::isT1BP_(TypeID t) const {
    // cannot use this because it is based on invention. A t1 blueprint without invention will not show up in it.
    // return t1_blueprints.find(t) != t1_blueprints.end();

    return isBP_(t) && !isT2BP_(t) && !isT3BP_(t) && !isRelic_(t);
}

bool Types::isT2BP_(TypeID t) const { return t2_blueprints.find(t) != t2_blueprints.end(); }

bool Types::isT1_(TypeID t) const {
    auto it = productToBP.find(t);
    return it != productToBP.end() && isT1BP_(it->second);
}

bool Types::isT2_(TypeID t) const {
    auto it = productToBP.find(t);
    return it != productToBP.end() && isT2BP_(it->second);
}

bool Types::isRelic_(TypeID t) const { return ancient_relics.find(t) != ancient_relics.end(); }

bool Types::isT3BP_(TypeID t) const { return t3_blueprints.find(t) != t3_blueprints.end(); }

bool Types::isT3_(TypeID t) const {
    auto it = productToBP.find(t);
    return it != productToBP.end() && isT3BP_(it->second);
}

bool Types::isProduct_(TypeID t) const { return manufacturingOutputs.find(t) != manufacturingOutputs.end(); }

bool Types::isBaseItem_(TypeID t) const { return !isBP_(t) && !isProduct_(t); }

void Types::checkItem(TypeID t) const {
    auto isbp = isBP_(t);
    auto ist1bp = isT1BP_(t);
    auto ist2bp = isT2BP_(t);
    auto ist3bp = isT3BP_(t);
    auto isrelic = isRelic_(t);
    auto ist1 = isT1_(t);
    auto ist2 = isT2_(t);
    auto ist3 = isT3_(t);
    auto isproduct = isProduct_(t);
    auto base = isBaseItem_(t);
    int a = base;
    int b = ist1 && isproduct;
    int c = ist2 && isproduct;
    int d = ist3 && isproduct;
    int e = ist1bp && isbp;
    int f = ist2bp && isbp;
    int g = ist3bp && isbp;
    int h = isrelic && isbp;
    auto ok = (a + b + c + d + e + f + g + h == 1);
    if (!ok) {
        std::cout << "typeid: " << t << '\n';
        std::cout << "bp: " << isbp << '\n';
        std::cout << "t1bp: " << ist1bp << '\n';
        std::cout << "t2bp: " << ist2bp << '\n';
        std::cout << "t3bp: " << ist3bp << '\n';
        std::cout << "relic: " << isrelic << '\n';
        std::cout << "t1: " << ist1 << '\n';
        std::cout << "t2: " << ist2 << '\n';
        std::cout << "t3: " << ist3 << '\n';
        std::cout << "product: " << isproduct << '\n';
        std::cout << "base: " << base << '\n';
        throw std::runtime_error("inconsistent item");
    }
}

bool Types::isBP(TypeID t) const {
    checkItem(t);
    return isBP_(t);
}

bool Types::isT1BP(TypeID t) const {
    checkItem(t);
    return isT1BP_(t);
}

bool Types::isT2BP(TypeID t) const {
    checkItem(t);
    return isT2BP_(t);
}

bool Types::isT3BP(TypeID t) const {
    checkItem(t);
    return isT3BP_(t);
}

TechLevel Types::getBPTechLevel(TypeID t) const {
    checkItem(t);
    if (isT1BP_(t))
        return TechLevel::T1;
    if (isT2BP_(t))
        return TechLevel::T2;
    if (isT3BP_(t))
        return TechLevel::T3;
    throw std::runtime_error("No blueprint: " + std::to_string(t));
}

TechLevel Types::getProductTechLevel(TypeID t) const {
    checkItem(t);
    if (isT1_(t))
        return TechLevel::T1;
    if (isT2_(t))
        return TechLevel::T2;
    if (isT3_(t))
        return TechLevel::T3;
    throw std::runtime_error("No product: " + std::to_string(t));
}

bool Types::isRelic(TypeID t) const {
    checkItem(t);
    return isRelic_(t);
}

bool Types::isT1(TypeID t) const {
    checkItem(t);
    return isT1_(t);
}

bool Types::isT2(TypeID t) const {
    checkItem(t);
    return isT2_(t);
}

bool Types::isT3(TypeID t) const {
    checkItem(t);
    return isT3_(t);
}

bool Types::isProduct(TypeID t) const {
    checkItem(t);
    return isProduct_(t);
}

bool Types::isBaseItem(TypeID t) const {
    checkItem(t);
    return isBaseItem_(t);
}

void Types::registerBlueprint(unsigned blueprintId, const YAML::Node& b, const Groups& groups) {
    std::cout << "registering blueprint " << blueprintId << '\n';
    blueprints.insert(blueprintId);
    const auto& activities = b["activities"];
    std::cout << "  manufacturing product: ";
    try {
        auto productId = activities["manufacturing"]["products"][0]["typeID"].as<unsigned>();
        manufacturingOutputs.insert(productId);
        bpToProduct[blueprintId] = productId;
        productToBP[productId] = blueprintId;
        std::cout << productId << "\n";
    } catch (const YAML::Exception& e) {
        std::cout << "-\n";
    }
    if (!is_ancient_relic(b, groups)) {
        std::cout << "  T2 invention:";
        for (const auto& productNode : get_invention_products(activities)) {
            try {
                auto productId = productNode["typeID"].as<unsigned>();
                t1_blueprints.insert(blueprintId);
                t2_blueprints.insert(productId);
                t2BPToT1BP[productId] = blueprintId;
                t1BPToT2BPs.emplace(blueprintId, productId);
                std::cout << " " << productId;
            } catch (const YAML::Exception& e) {
                std::cout << "-\n";
            }
        }
        std::cout << '\n';
    } else {
        std::cout << "  T3 invention:";
        for (const auto& productNode : get_invention_products(activities)) {
            try {
                auto productId = productNode["typeID"].as<unsigned>();
                ancient_relics.insert(blueprintId);
                t3_blueprints.insert(productId);
                t3BPToRelic[productId] = blueprintId;
                relicToT3BPs.emplace(blueprintId, productId);
                std::cout << " " << productId;
            } catch (const YAML::Exception& e) {
                std::cout << "-\n";
            }
        }
        std::cout << '\n';
    }
}

YAML::Node Types::get_invention_products(const YAML::Node& activities) {
    try {
        return activities["invention"]["products"];
    } catch (const YAML::InvalidNode&) {
        return YAML::Node(YAML::NodeType::Sequence);
    }
}

YAML::Node Types::get_manufacturing_products(const YAML::Node& activities) {
    try {
        return activities["manufacturing"]["products"];
    } catch (const YAML::InvalidNode&) {
        return YAML::Node(YAML::NodeType::Sequence);
    }
}

bool Types::is_ancient_relic(const YAML::Node& root, const Groups& groups) {
    static const std::array<TypeID, 6> relicIDs{{971, 990, 991, 992, 993, 997}};
    auto group = groups.checkGroup(root["blueprintTypeID"].as<unsigned>());
    return group != nullptr && std::binary_search(relicIDs.begin(), relicIDs.end(), *group);
    // try {
    //    for (const auto& skill : root["activities"]["invention"]["skills"])
    //        if (skill["typeID"].as<int>() == 3408)
    //            return true;
    //    return false;
    //} catch(const YAML::Exception&) {
    //    return false;
    //}
}
