#include "jobs.h"

#include <copies.h>
#include <inventions.h>
#include <products.h>
#include <unordered_map>

struct Skills;

void Jobs::reserve(const std::size_t) {}

void Jobs::filter(const Skills& skills,
                  const std::unordered_map<std::size_t, double>& avgPrices,
                  const double bpoPriceLimit) {
    Products::filter(skills);
    Copies::filter(avgPrices, bpoPriceLimit);
    Inventions::filter(skills);
}
