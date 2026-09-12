#include "grid/layout/GridLayout.hpp"

#include "fixtures/FixtureCheck.hpp"

#include <cmath>
#include <limits>

int main()
{
    const blitzar_grid::GridLayout invalid;

    BLITZAR_CHECK(!invalid.IsValid());
    BLITZAR_CHECK(invalid.CellCount() == 0);

    const blitzar_grid::GridLayout layout({2, 3, 4}, {0.0, -1.0, 2.0}, {2.0, 2.0, 6.0});

    BLITZAR_CHECK(layout.IsValid());
    BLITZAR_CHECK(layout.Dimensions().x == 2);
    BLITZAR_CHECK(layout.Dimensions().y == 3);
    BLITZAR_CHECK(layout.Dimensions().z == 4);
    BLITZAR_CHECK(layout.CellCount() == 24);
    BLITZAR_CHECK(layout.FlatIndex({0, 0, 0}) == 0);
    BLITZAR_CHECK(layout.FlatIndex({1, 2, 3}) == 23);
    BLITZAR_CHECK(layout.FlatIndex({2, 0, 0}) == std::numeric_limits<std::size_t>::max());

    const blitzar_core::Vector3 first = layout.CellCenter(0);
    const blitzar_core::Vector3 last = layout.CellCenter(23);

    BLITZAR_CHECK(first.x == 0.5 && first.y < 0.0 && first.z == 2.5);
    BLITZAR_CHECK(last.x == 1.5 && last.y > 1.0 && last.z == 5.5);
    BLITZAR_CHECK(layout.CellCenter(24).x == 0.0);

    blitzar_grid::GridInterpolation interpolation{};

    BLITZAR_CHECK(layout.Locate({-10.0, 10.0, 4.0}, interpolation));
    BLITZAR_CHECK(interpolation.lower.x == 0 && interpolation.lower.y == 2);
    BLITZAR_CHECK(interpolation.upper.x == 0 && interpolation.upper.y == 2);
    BLITZAR_CHECK(interpolation.lower.z == 1 && interpolation.upper.z == 2);
    BLITZAR_CHECK(interpolation.upper_weight.z == 0.5);

    BLITZAR_CHECK(
        !layout.Locate({std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0}, interpolation));

    return 0;
}
