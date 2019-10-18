#include "libs/industry_resources/resource_base.h"

#include <infinity.h>

// ResourceManager* ResourceManager::current_{};
Resource_Base::Resource_Base(std::string n, std::string fn, double l)
    : lower_limit(l), upper_limit(infinity), name(std::move(n)), fullName(std::move(fn)) {}

Resource_Base::~Resource_Base() = default;
