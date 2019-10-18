#pragma once

#include <cstdint>
#include <string>

struct Order {
    Order() = default;
    Order(std::size_t i, std::size_t t, double p, std::size_t v, std::size_t stationID_, std::string stationName_);

    std::size_t id;
    std::size_t type;
    double price;
    std::size_t volume;
    std::size_t stationID;
    std::string stationName;
};

bool operator==(const Order&, const Order&);
