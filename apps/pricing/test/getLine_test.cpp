#include "getLine.h"

#include <catch.hpp>

//TEST_CASE("empty string", "[getLine]") {
//    auto s = "";
//    REQUIRE_THROWS(getLine(s, 0));
//}
//
//TEST_CASE("default case", "[getLine]") {
//    std::string_view s("abc\ndef\nghi");
//
//    REQUIRE(s.rfind('\n', 1) == std::string_view::npos);
//    REQUIRE(s.find('\n', 1) == 3);
//
//    REQUIRE(s.rfind('\n', 5) == 3);
//    REQUIRE(s.find('\n', 5) == 7);
//
//    REQUIRE(s.rfind('\n', 9) == 7);
//    REQUIRE(s.find('\n', 9) == std::string_view::npos);
//
//    REQUIRE(getLine(s, 0) == "abc");
//    REQUIRE(getLine(s, 2) == "abc");
//    REQUIRE_THROWS(getLine(s, 3));
//    REQUIRE(getLine(s, 4) == "def");
//
//    REQUIRE(getLine(s, 10) == "ghi");
//
//    // ... again here the offset points to a '\n' and we fail ...
//    auto s2 = "}\n";
//    REQUIRE_THROWS(getLine(s2, 1));
//}
