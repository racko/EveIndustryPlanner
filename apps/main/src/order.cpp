#include "order.h"

Order::Order(size_t i, size_t t, double p, size_t v, size_t stationID_, std::string stationName_) : id(i), type(t), price(p), volume(v), stationID(stationID_), stationName(std::move(stationName_)) {}
