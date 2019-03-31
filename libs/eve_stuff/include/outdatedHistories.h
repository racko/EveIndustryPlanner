#pragma once
#include <vector>
#include <cstddef>
#include "sqlite.h"


std::vector<std::size_t> outdatedHistories(const sqlite::dbPtr& db);
