#pragma once

#include "type_id.h"

#include <chrono>
#include <cstddef>
#include <utility>
#include <vector>

class Planetary {
public:
    using type_t = TypeID;
    using product_t = type_t;
    using materials_t = std::vector<std::pair<type_t,std::size_t>>;
    using duration_t = std::chrono::seconds;
    using tier_t = std::uint8_t;

    Planetary(product_t product_, std::size_t producedQuantity_, materials_t materials_, duration_t duration_, tier_t tier_) : product(product_), producedQuantity(producedQuantity_), materials(std::move(materials_)), duration(duration_), tier(tier_) {}

    product_t getProduct() const { return product; }
    std::size_t getProducedQuantity() const { return producedQuantity; }
    const materials_t& getMaterials() const { return materials; }
    duration_t getDuration() const { return duration; }
    tier_t getTier() const { return tier; }
private:
    product_t product;
    std::size_t producedQuantity;
    materials_t materials;
    duration_t duration;
    tier_t tier;
};
