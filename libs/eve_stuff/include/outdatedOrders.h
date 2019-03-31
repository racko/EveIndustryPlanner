#pragma once
#include <vector>
#include <cstddef>
#include "sqlite.h"


std::vector<size_t> outdatedOrders(const sqlite::dbPtr& db);
std::vector<size_t> outdatedOrders2(const sqlite::dbPtr& db, size_t region);
