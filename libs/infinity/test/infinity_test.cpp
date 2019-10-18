#include <infinity.h>

#include <catch2/catch.hpp>
#include <soplex.h>

TEST_CASE("infinity needs to equal soplex::infinity", "[infinity]") { REQUIRE(infinity == soplex::infinity); }
