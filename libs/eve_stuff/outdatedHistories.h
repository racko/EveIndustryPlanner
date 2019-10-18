#pragma once
#include "sqlite.h"
#include <cstddef>
#include <vector>

std::vector<std::size_t> outdatedHistories(const sqlite::dbPtr& db);
