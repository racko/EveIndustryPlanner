#include "libs/market_data/order.h"

Order::Order(std::size_t i, std::size_t t, double p, std::size_t v, std::size_t stationID_, std::string stationName_)
    : id(i), type(t), price(p), volume(v), stationID(stationID_), stationName(std::move(stationName_)) {}

bool operator==(const Order& a, const Order& b) {
    return a.id == b.id && a.type == b.type && a.price == b.price && a.volume == b.volume &&
           a.stationID == b.stationID && a.stationName == b.stationName;
}
