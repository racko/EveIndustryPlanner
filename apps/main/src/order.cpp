#include "order.h"

Order::Order(std::size_t i, std::size_t t, double p, std::size_t v, std::size_t stationID_, std::string stationName_) : id(i), type(t), price(p), volume(v), stationID(stationID_), stationName(std::move(stationName_)) {}
