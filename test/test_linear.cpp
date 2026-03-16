#include <catch2/catch_test_macros.hpp>
#include <vector>
#include <memory>

#include "interpolation.hpp"

TEST_CASE("Linear interpolation", "[linear]") {
    auto x = std::make_shared<const std::vector<double>>(
        std::vector<double>{0.0, 1.0, 2.0}
    );
    auto y = std::make_shared<const std::vector<double>>(
        std::vector<double>{0.0, 1.0, 2.0}
    );

    interpolation::LinearInterpolator<double> interp(x, y);

    REQUIRE(interp.eval(0.0) == 0.0);
    REQUIRE(interp.eval(0.5) == 0.5);
    REQUIRE(interp.eval(1.5) == 1.5);
    REQUIRE(interp.eval(2.0) == 2.0);
}
