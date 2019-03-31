#pragma once

#include <cstddef>
#include <unordered_map>

struct Jobs;
struct Skills;

void filter_jobs(Jobs& jobs, const Skills& skills, const std::unordered_map<std::size_t, double>& avgPrices, double bpoPriceLimit);
