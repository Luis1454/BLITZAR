#include "integration/comoving/ComovingBackground.hpp"

#include "core/CoreExecution.hpp"
#include "fixtures/FixtureCheck.hpp"

#include <cmath>
#include <cstdint>
#include <limits>

namespace {

int CheckStaticFrame() noexcept
{
    const blitzar_integration::ComovingBackground background{};

    BLITZAR_CHECK(background.IsValid());
    BLITZAR_CHECK(blitzar_integration::ScaleFactor(background, 42.0) == 1.0);
    BLITZAR_CHECK(blitzar_integration::ExpansionRate(background, 42.0) == 0.0);
    BLITZAR_CHECK(blitzar_integration::Redshift(background, 42.0) == 0.0);
    BLITZAR_CHECK(blitzar_integration::ComovingDrift(background, 42.0, 0.5) == 0.5);

    return 0;
}

int CheckEdsScaleFactor() noexcept
{
    const blitzar_integration::ComovingBackground background{
        blitzar_integration::ExpansionKind::EinsteinDeSitter, 1.0};

    BLITZAR_CHECK(background.IsValid());
    BLITZAR_CHECK(std::abs(blitzar_integration::ScaleFactor(background, 1.0) - 1.0) < 1.0e-12);
    BLITZAR_CHECK(std::abs(blitzar_integration::ScaleFactor(background, 8.0) - 4.0) < 1.0e-12);
    BLITZAR_CHECK(std::abs(blitzar_integration::ScaleFactor(background, 0.125) - 0.25) < 1.0e-12);

    return 0;
}

int CheckEdsExpansionRate() noexcept
{
    const blitzar_integration::ComovingBackground background{
        blitzar_integration::ExpansionKind::EinsteinDeSitter, 1.0};

    BLITZAR_CHECK(
        std::abs(blitzar_integration::ExpansionRate(background, 1.0) - (2.0 / 3.0)) < 1.0e-12);

    BLITZAR_CHECK(
        std::abs(blitzar_integration::ExpansionRate(background, 8.0) - (2.0 / 24.0)) < 1.0e-12);

    BLITZAR_CHECK(
        std::abs(blitzar_integration::ExpansionRate(background, 0.125) - (2.0 / 0.375)) < 1.0e-12);

    return 0;
}

int CheckEdsRedshift() noexcept
{
    const blitzar_integration::ComovingBackground background{
        blitzar_integration::ExpansionKind::EinsteinDeSitter, 1.0};

    BLITZAR_CHECK(std::abs(blitzar_integration::Redshift(background, 1.0)) < 1.0e-12);
    BLITZAR_CHECK(std::abs(blitzar_integration::Redshift(background, 8.0) + 0.75) < 1.0e-12);
    BLITZAR_CHECK(std::abs(blitzar_integration::Redshift(background, 0.125) - 3.0) < 1.0e-12);

    return 0;
}

int CheckEdsDrift() noexcept
{
    const blitzar_integration::ComovingBackground background{
        blitzar_integration::ExpansionKind::EinsteinDeSitter, 1.0};

    BLITZAR_CHECK(
        std::abs(blitzar_integration::ComovingDrift(background, 1.0, 7.0) - 3.0) < 1.0e-12);

    BLITZAR_CHECK(
        std::abs(blitzar_integration::ComovingDrift(background, 8.0, 19.0) - 3.0) < 1.0e-12);

    const double tiny_drift = blitzar_integration::ComovingDrift(background, 8.0, 1.0e-9);
    const double scaled_reference = 1.0e-9 / blitzar_integration::ScaleFactor(background, 8.0);

    BLITZAR_CHECK(std::abs(tiny_drift - scaled_reference) / scaled_reference < 1.0e-4);

    return 0;
}

int CheckInvalidBackground() noexcept
{
    const blitzar_integration::ComovingBackground static_invalid{
        blitzar_integration::ExpansionKind::Static, 0.0};

    const blitzar_integration::ComovingBackground eds_invalid{
        blitzar_integration::ExpansionKind::EinsteinDeSitter, -1.0};

    const blitzar_integration::ComovingBackground eds_infinite{
        blitzar_integration::ExpansionKind::EinsteinDeSitter,
        std::numeric_limits<double>::infinity()};

    const blitzar_integration::ComovingBackground unknown_kind{
        static_cast<blitzar_integration::ExpansionKind>(42), 1.0};

    BLITZAR_CHECK(!static_invalid.IsValid());
    BLITZAR_CHECK(!eds_invalid.IsValid());
    BLITZAR_CHECK(!eds_infinite.IsValid());
    BLITZAR_CHECK(!unknown_kind.IsValid());

    return 0;
}

} // namespace

int main()
{
    BLITZAR_CHECK(CheckStaticFrame() == 0);
    BLITZAR_CHECK(CheckEdsScaleFactor() == 0);
    BLITZAR_CHECK(CheckEdsExpansionRate() == 0);
    BLITZAR_CHECK(CheckEdsRedshift() == 0);
    BLITZAR_CHECK(CheckEdsDrift() == 0);
    BLITZAR_CHECK(CheckInvalidBackground() == 0);

    return 0;
}
