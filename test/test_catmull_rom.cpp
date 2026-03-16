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

    REQUIRE(h.eval(0.0) == Catch::Approx(0.0));
    REQUIRE(h.eval(1.0) == Catch::Approx(1.0));
    REQUIRE(h.eval(2.0) == Catch::Approx(2.0));
    REQUIRE(h.eval(3.0) == Catch::Approx(3.0));
}
