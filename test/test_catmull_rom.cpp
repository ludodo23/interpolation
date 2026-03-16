#include <catch2/catch_test_macros.hpp>
#include <vector>
#include <memory>

#include "interpolation.hpp"

TEST_CASE("Catmull-Rom spline", "[catmull-rom]") {
    auto x = std::make_shared<const std::vector<double>>(
        std::vector<double>{0.0, 1.0, 2.0, 3.0}
    );
    auto y = std::make_shared<const std::vector<double>>(
        std::vector<double>{0.0, 1.0, 2.0, 3.0}
    );

    interpolation::CatmullRomInterpolator<double> cr(x, y);

    double v = cr.eval(1.5);

    REQUIRE(v > 1.0);
    REQUIRE(v < 2.0);
}
