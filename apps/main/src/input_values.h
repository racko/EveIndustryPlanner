#pragma once

#include <cstddef>
#include <unordered_map>
#include <vector>

class Production;

std::unordered_map<size_t,double> inputValues(const std::vector<Production>& products, const std::unordered_map<size_t, double>& adjPrices);
