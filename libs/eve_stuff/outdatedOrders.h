#pragma once
#include "sqlite.h"
#include <cstddef>
#include <vector>

std::vector<std::size_t> outdatedOrders(const sqlite::dbPtr& db);
std::vector<std::size_t> outdatedOrders2(const sqlite::dbPtr& db, std::size_t region);
