#include "interpolation.hpp"

int main() {

    auto x = std::make_shared<const std::vector<double>>(
        std::vector<double>{0.0, 1.0, 2.0, 3.0}
    );
    auto y = std::make_shared<const std::vector<double>>(
        std::vector<double>{0.0, 1.0, 2.0, 3.0}
    );

    interpolation::CatmullRomInterpolator<double> cr(x, y);

    cr.eval(2.5);

    return 0;
}