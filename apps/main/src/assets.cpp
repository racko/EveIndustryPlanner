#include "assets.h"
#include "names.h"

#include <iostream>
#include <fstream>
#include <pugixml.hpp>
#include <sstream>

void Assets::loadAssetsFromXml(const Names& names) {
    assets.reserve(1000);
    const auto filename = "assetlist.xml";
    std::cout << "Assets (" << filename << "):\n";
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(filename);
    if (!result) {
        std::cout << "Error description: " << result.description() << "\n";
        std::cout << "Error offset: " << result.offset << "\n\n";
        throw std::runtime_error("parsing asset list failed.");
    }
    static const auto assetsString = std::string("assets");
    const auto& assetlist = doc.child("eveapi").child("result").find_child([] (const pugi::xml_node& n) { return n.attribute("name").as_string() == assetsString; }).find_child([] (const pugi::xml_node& n) { return n.attribute("itemID").as_ullong() == 1019171856618; }).child("rowset").children();
    for (const auto& asset : assetlist) {
        auto typeId = asset.attribute("typeID").as_ullong();
        auto quantity = asset.attribute("quantity").as_ullong();
        std::cout << "  " << names.getName(typeId) << " (" << typeId << "): " << quantity << '\n';
        auto assetIt = assets.insert({typeId, 0}); // no-op if it already exists.
        assetIt.first->second += quantity;
    }
}

void Assets::addAssetsFromTsv(const char* filename, const Names& names) {
    std::cout << "Assets (" << filename << "):\n";
    std::ifstream file(filename);
    std::string line;
    while(std::getline(file, line)) {
        std::stringstream linestream(line);
        std::string name, quantity_str;
        if (!std::getline(linestream, name, '\t') || !std::getline(linestream, quantity_str, '\t'))
            throw std::runtime_error("Failed to parse assets: '" + line + "'");
        auto typeId = names.getTypeId(name);
        std::stringstream qstr(quantity_str); // TODO: avoid unnecessary copies
        qstr.imbue(std::locale("en_US.UTF-8")); // allows parsing "1,000,000" etc.
        std::size_t quantity;
        qstr >> quantity;
        auto assetIt = assets.insert({typeId, 0}); // no-op if it already exists.
        assetIt.first->second += quantity;
        std::cout << "  " << names.getName(typeId) << " (" << typeId << "): +" << quantity << " -> " << assetIt.first->second << '\n';
    }
}

void Assets::loadAssetsFromTsv(const Names& names) {
    assets.reserve(1000);
    addAssetsFromTsv("materials.txt", names);
    addAssetsFromTsv("products.txt", names);
}

void Assets::loadOwnedBPCsFromXml(const Names& names) {
    std::cout << "Owned blueprints:\n";
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file("blueprints.xml");
    if (!result) {
        std::cout << "Error description: " << result.description() << "\n";
        std::cout << "Error offset: " << result.offset << "\n\n";
        throw std::runtime_error("parsing blueprints failed.");
    }
    static const auto assetsString = std::string("blueprints");
    const auto& owned_blueprints = doc.child("eveapi").child("result").find_child([] (const pugi::xml_node& n) { return n.attribute("name").as_string() == assetsString; }).children();
    ownedBPCs.reserve(std::size_t(std::distance(owned_blueprints.begin(), owned_blueprints.end())));
    for (const auto& bp : owned_blueprints) {
        auto typeId = bp.attribute("typeID").as_ullong();
        auto quantity = bp.attribute("quantity").as_llong();
        if (quantity != -2) // bpo's have -2 instead of bpc runs
            continue;
        auto runs = bp.attribute("runs").as_ullong();
        auto te = 0.01 * double(100u - bp.attribute("timeEfficiency").as_ullong());
        auto me = 0.01 * double(100u - bp.attribute("materialEfficiency").as_ullong());
        auto bpe = BlueprintEfficiency(me, te);
        auto x = BlueprintWithEfficiency(typeId, bpe);
        auto bpIt = ownedBPCs.insert({x, 0}); // no-op if it already exists.
        bpIt.first->second += runs;
        std::cout << "  " << names.getName(typeId) << " " << bpe << " (" << typeId << "): +" << runs << " -> " << bpIt.first->second << '\n';
    }
}

std::size_t Assets::countAsset(std::size_t typeId) const {
    auto a = assets.find(typeId);
    return a != assets.end() ? a->second : 0;
}

std::size_t Assets::countPlanetaryAsset(std::size_t typeId) const {
    auto a = planetary_assets.find(typeId);
    return a != planetary_assets.end() ? a->second : 0;
}

std::size_t Assets::runs(const BlueprintWithEfficiency& bpwe) const {
    auto r = ownedBPCs.find(bpwe);
    return r != ownedBPCs.end() ? r->second : 0;
}

std::size_t Assets::runs(std::size_t typeId) const {
    auto runs = 0ul;
    for (const auto& ownedBP : ownedBPCs)
        if (ownedBP.first.getTypeID() == typeId)
            runs += ownedBP.second;
    return runs;
}
