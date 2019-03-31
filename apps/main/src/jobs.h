#pragma once

#include "production.h"
#include "copying.h"
#include "research.h"
#include "planetary.h"
#include "yaml_fwd.h"

#include <unordered_map>
#include <vector>

struct Names;
struct Skills;

struct Jobs {
    using products_t = std::vector<Production>;
    using copies_t = std::vector<Copying>;
    using research_t = std::vector<Research>;
    using schematics_t = std::vector<Planetary>;
    void loadSchematics(const Names& names);

    void addManufacturingData(const YAML::Node& manufacturing, unsigned blueprintId, const Names& names, const std::unordered_map<std::size_t, double>& avgPrices, const Skills& skills);

    void addInventionData(const YAML::Node& invention, unsigned blueprintId, const Names& names, const std::unordered_map<std::size_t, double>& avgPrices, const Skills& skills);

    void addCopyingData(const YAML::Node& copying, unsigned blueprintId, const Names& names, const std::unordered_map<std::size_t, double>& avgPrices);

    const products_t& getProducts() const { return products; }
    products_t& getProducts() { return products; }

    const copies_t& getT1Copies() const { return t1Copies; }
    copies_t& getT1Copies() { return t1Copies; }

    const research_t& getT2Blueprints() const { return t2Blueprints; }
    research_t& getT2Blueprints() { return t2Blueprints; }

    const research_t& getT3Blueprints() const { return t3Blueprints; }
    research_t& getT3Blueprints() { return t3Blueprints; }

    const schematics_t& getSchematics() const { return schematics; }
    schematics_t& getSchematics() { return schematics; }

    void reserve(std::size_t n);

private:
    products_t products;
    copies_t t1Copies;
    research_t t2Blueprints;
    research_t t3Blueprints;
    schematics_t schematics;
};

// TODO:
// 1. create jobs "unfiltered". Don't compute inputValues in Jobs.
// 2. create inputValues (using input_values.h)
// 3. filter jobs (TODO: add "Jobs filter(Jobs, predicates)")
