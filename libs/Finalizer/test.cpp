#include "libs/Finalizer/Finalizer.h"

#include <catch2/catch.hpp>

TEST_CASE("Finalizer default") {
    int finalized = 0;
    {
        Finalizer finalizer{[&] { ++finalized; }};
    }
    REQUIRE(finalized == 1);
}

TEST_CASE("Finalizer abort") {
    bool finalized = false;
    {
        Finalizer finalizer{[&] { finalized = true; }};
        finalizer.abort();
    }
    REQUIRE(!finalized);
}

struct Dummy {
    void operator()();
};

// otherwise: test
static_assert(!std::is_move_constructible_v<Finalizer<Dummy>>);
static_assert(!std::is_move_assignable_v<Finalizer<Dummy>>);
static_assert(!std::is_copy_constructible_v<Finalizer<Dummy>>);
static_assert(!std::is_copy_assignable_v<Finalizer<Dummy>>);
