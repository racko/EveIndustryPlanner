#include "libs/industry_resources/production.h"

Production::Production(blueprint_t blueprint_,
                       product_t product_,
                       std::size_t producedQuantity_,
                       materials_t materials_,
                       skills_t skills_,
                       duration_t duration_)
    : blueprint(blueprint_),
      product(product_),
      producedQuantity(producedQuantity_),
      materials(std::move(materials_)),
      skills(std::move(skills_)),
      duration(duration_) {}
