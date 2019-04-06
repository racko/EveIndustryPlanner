#pragma once

#include "job.h"
#include "resource_base.h"

#include <cstddef>
#include <string>

struct Buy : public Job {
    Buy(Resource_Base::Ptr r,
        std::size_t id,
        double b,
        double l,
        std::size_t stationID_,
        const std::string& stationName_)
        : Job({},
              {{r, 1}},
              b,
              "Buy_" + r->getName() + "_" + std::to_string(id),
              "Buy " + r->getFullName() + " at " + stationName_,
              0,
              l),
          resource(std::move(r)), stationID(stationID_), stationName(stationName_) {}
    Resource_Base::Ptr resource;
    std::size_t stationID;
    std::string stationName;
};
