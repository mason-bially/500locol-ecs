#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"

#include "dsecs.hpp"

using namespace dsecs;

struct TestComponentA {
    size_t a_number;
};

TEST_CASE("Component Managers are Stable", "[components]" ) {
    World w;

    auto a0 = w.requireComponent<TestComponentA>();
    auto a1 = w.requireComponent<TestComponentA>();

    REQUIRE( a0 != nullptr );
    REQUIRE( a0 == a1 );
}
