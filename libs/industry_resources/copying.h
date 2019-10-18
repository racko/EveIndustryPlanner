#pragma once

#include "libs/industry_resources/type_id.h"

#include <chrono>

class Copying {
  public:
    using type_t = TypeID;
    using blueprint_t = type_t;
    using duration_t = std::chrono::seconds;

    Copying(blueprint_t blueprint_, duration_t duration_) : blueprint(blueprint_), duration(duration_) {}

    blueprint_t getBlueprint() const { return blueprint; }
    duration_t getDuration() const { return duration; }

  private:
    blueprint_t blueprint;
    duration_t duration;
};
