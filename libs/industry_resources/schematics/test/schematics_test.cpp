#include "libs/industry_resources/schematics/schematics.h"

#include <catch2/catch.hpp>

TEST_CASE("0 <= tier <= 3", "[schematics]") {
    Schematics schematic_loader;
    schematic_loader.loadSchematics();
    const auto& schematics = schematic_loader.getSchematics();
    for (const auto& schematic : schematics) {
        REQUIRE(0 <= schematic.getTier());
        REQUIRE(schematic.getTier() <= 3);
    }
}
