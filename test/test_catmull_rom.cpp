#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
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

    REQUIRE(cr.eval(0.0) == Catch::Approx(0.0));
    REQUIRE(cr.eval(1.0) == Catch::Approx(1.0));
    REQUIRE(cr.eval(2.0) == Catch::Approx(2.0));
    REQUIRE(cr.eval(3.0) == Catch::Approx(3.0));
}
