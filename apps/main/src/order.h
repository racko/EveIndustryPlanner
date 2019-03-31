#pragma once

#include <cstdint>
#include <string>

struct Order {
    Order(size_t i, size_t t, double p, size_t v, size_t stationID_, std::string stationName_);
    size_t id;
    size_t type;
    double price;
    size_t volume;
    size_t stationID;
    std::string stationName;
};
