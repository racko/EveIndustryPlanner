#pragma once

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/serialization.hpp>
#include <order.h>

namespace boost {
namespace serialization {

template<class Archive>
void serialize(Archive& ar, Order& o, unsigned int)
{
    ar & o.id;
    ar & o.type;
    ar & o.price;
    ar & o.volume;
    ar & o.stationID;
    ar & o.stationName;
}

} // namespace serialization
} // namespace boost
