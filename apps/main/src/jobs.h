#pragma once

#include <copies.h>
#include <inventions.h>
#include <products.h>
#include <schematics.h>
#include <unordered_map>

struct Skills;

struct Jobs : private Copies, private Products, private Inventions, private Schematics {
    using Copies::addCopyingData;
    using Copies::getCopyJobs;

    using Inventions::addInventionData;
    using Inventions::getT2InventionJobs;
    using Inventions::getT3InventionJobs;

    using Products::addManufacturingData;
    using Products::getManufacturingJobs;
    using Products::getProducts;

    using Schematics::getPlanetaryJobs;
    using Schematics::loadSchematics;

    void reserve(std::size_t n);

    void filter(const Skills& skills, const std::unordered_map<std::size_t, double>& avgPrices, double bpoPriceLimit);
};

// TODO:
// 1. create jobs "unfiltered". Don't compute inputValues in Jobs.
// 2. create inputValues (using input_values.h)
// 3. filter jobs (TODO: add "Jobs filter(Jobs, predicates)")
