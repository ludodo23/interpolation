#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <vector>
#include <memory>
#include <cmath>

#include "interpolation.hpp"

TEST_CASE("Cubic Hermite interpolation", "[hermite]") {
    auto x = std::make_shared<const std::vector<double>>(
        std::vector<double>{0.0, 1.0, 2.0}
    );
    auto y = std::make_shared<const std::vector<double>>(
        std::vector<double>{0.0, 1.0, 0.0}
    );
    auto dy = std::make_shared<const std::vector<double>>(
        std::vector<double>{1.0, 0.0, -1.0}
    );

    interpolation::CubicHermiteInterpolator<double> h(x, y, dy);

    REQUIRE(h.eval(0.0) == Catch::Approx(0.0));
    REQUIRE(h.eval(1.0) == Catch::Approx(1.0));
    REQUIRE(h.eval(2.0) == Catch::Approx(0.0));
}
