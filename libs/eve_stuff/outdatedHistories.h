#pragma once
#include "libs/sqlite-util/sqlite.h"
#include <cstddef>
#include <vector>

std::vector<std::size_t> outdatedHistories(const sqlite::dbPtr& db);
