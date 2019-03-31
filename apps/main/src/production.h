#pragma once

#include "type_id.h"

#include <chrono>
#include <cstddef>
#include <utility>
#include <vector>

class Production {
public:
    using type_t = TypeID;
    using skill_t = unsigned;
    using level_t = unsigned;
    using skill_requirement_t = std::pair<skill_t,level_t>;
    using blueprint_t = type_t;
    using product_t = type_t;
    using materials_t = std::vector<std::pair<type_t,std::size_t>>;
    using skills_t = std::vector<skill_requirement_t>;
    using duration_t = std::chrono::duration<double>;

    Production(blueprint_t blueprint_, product_t product_, std::size_t producedQuantity_, materials_t materials_, skills_t skills_, duration_t duration_);

    blueprint_t getBlueprint() const { return blueprint; }
    product_t getProduct() const { return product; }
    std::size_t getProducedQuantity() const { return producedQuantity; }
    const materials_t& getMaterials() const { return materials; }
    const skills_t& getSkills() const { return skills; }
    duration_t getDuration() const { return duration; }
private:
    blueprint_t blueprint;
    product_t product;
    std::size_t producedQuantity;
    materials_t materials;
    skills_t skills;
    duration_t duration;
};
