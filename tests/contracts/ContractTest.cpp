#include "core/CoreExecution.hpp"
#include "core/CoreSnapshot.hpp"
#include "core/CoreStatus.hpp"
#include "core/CoreUnits.hpp"
#include "fixtures/FixtureCheck.hpp"
#include "physics/gravity/GravityLaw.hpp"
#include "solvers/SolverContract.hpp"
#include "solvers/SolverForceEvaluation.hpp"

#include <blitzar/blitzar.hpp>
#include <cmath>
#include <span>
#include <utility>

namespace {

class ProbeSolver final {
public:
    [[nodiscard]] blitzar_solvers::SolverKind Kind() const noexcept
    {
        return blitzar_solvers::SolverKind::Direct;
    }

    [[nodiscard]] blitzar_status Evaluate(
        const blitzar_solvers::SolverForceEvaluation& request) noexcept
    {
        if (!blitzar_core::IsValid(request.targets) || !blitzar_core::IsValid(request.forces) ||
            request.targets.count != request.forces.count || !request.settings.IsValid()) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        return BLITZAR_STATUS_OK;
    }
};

} // namespace

int main()
{
    const blitzar_core::ExecutionSettings settings{};

    BLITZAR_CHECK(settings.IsValid());
    BLITZAR_CHECK(blitzar::FromCStatus(BLITZAR_STATUS_OK) == blitzar::Status::Ok);
    BLITZAR_CHECK(
        blitzar::FromCStatus(BLITZAR_STATUS_INVALID_ARGUMENT) == blitzar::Status::InvalidArgument);

    BLITZAR_CHECK(blitzar::FromCStatus(BLITZAR_STATUS_ALLOCATION_FAILURE) ==
                  blitzar::Status::AllocationFailure);

    BLITZAR_CHECK(
        blitzar::FromCStatus(BLITZAR_STATUS_INTERNAL_ERROR) == blitzar::Status::InternalError);

    BLITZAR_CHECK(blitzar::FromCStatus(BLITZAR_STATUS_SINGULARITY) == blitzar::Status::Singularity);
    BLITZAR_CHECK(blitzar::FromCStatus(BLITZAR_STATUS_UNSUPPORTED) == blitzar::Status::Unsupported);
    BLITZAR_CHECK(
        blitzar::FromCStatus(static_cast<blitzar_status>(999)) == blitzar::Status::InternalError);

    BLITZAR_CHECK(blitzar_core::ToPublicStatus(999) == BLITZAR_STATUS_INTERNAL_ERROR);
    BLITZAR_CHECK(
        blitzar_core::ToPublicStatus(BLITZAR_STATUS_UNSUPPORTED) == BLITZAR_STATUS_UNSUPPORTED);

    blitzar_core::UnitSystem units{};

    BLITZAR_CHECK(units.IsValid());

    units.length_scale = 0.0;

    BLITZAR_CHECK(!units.IsValid());

    const blitzar_physics::GravityParameters scaled_gravity{1.0, 0.0, {2.0, 3.0, 4.0}};

    BLITZAR_CHECK(scaled_gravity.IsValid());
    BLITZAR_CHECK(std::abs(scaled_gravity.EffectiveConstant() - 6.0) < 1.0e-12);

    const blitzar_physics::GravityLaw scaled_law(scaled_gravity);

    BLITZAR_CHECK(std::abs(scaled_law.PairFactor(1.0, 1.0) - 6.0) < 1.0e-12);

    blitzar_core::SnapshotHeader snapshot{};

    snapshot.particle_count = 4;

    BLITZAR_CHECK(snapshot.IsCompatible());

    snapshot.magic = 0;

    BLITZAR_CHECK(!snapshot.IsCompatible());

    ProbeSolver solver{};
    blitzar_core::Scalar x[1]{};
    blitzar_core::Scalar y[1]{};
    blitzar_core::Scalar z[1]{};
    blitzar_core::Scalar vx[1]{};
    blitzar_core::Scalar vy[1]{};
    blitzar_core::Scalar vz[1]{};
    blitzar_core::Scalar mass[1]{1.0};
    blitzar_core::Scalar fx[1]{};
    blitzar_core::Scalar fy[1]{};
    blitzar_core::Scalar fz[1]{};
    const blitzar_core::ParticleStateView particles{1, std::span<const blitzar_core::Scalar>(x),
        std::span<const blitzar_core::Scalar>(y), std::span<const blitzar_core::Scalar>(z),
        std::span<const blitzar_core::Scalar>(vx), std::span<const blitzar_core::Scalar>(vy),
        std::span<const blitzar_core::Scalar>(vz), std::span<const blitzar_core::Scalar>(mass)};

    const blitzar_core::ForceView forces{1, std::span<blitzar_core::Scalar>(fx),
        std::span<blitzar_core::Scalar>(fy), std::span<blitzar_core::Scalar>(fz)};

    BLITZAR_CHECK(solver.Kind() == blitzar_solvers::SolverKind::Direct);
    BLITZAR_CHECK(solver.Evaluate({particles, particles, forces, settings,
                      blitzar_solvers::SolverForceSourceKind::Local}) == BLITZAR_STATUS_OK);

    const blitzar_core::ParticleStateView invalid{1, {}, std::span<const blitzar_core::Scalar>(y),
        std::span<const blitzar_core::Scalar>(z), std::span<const blitzar_core::Scalar>(vx),
        std::span<const blitzar_core::Scalar>(vy), std::span<const blitzar_core::Scalar>(vz),
        std::span<const blitzar_core::Scalar>(mass)};

    BLITZAR_CHECK(
        solver.Evaluate({invalid, invalid, forces, settings,
            blitzar_solvers::SolverForceSourceKind::Local}) == BLITZAR_STATUS_INVALID_ARGUMENT);

    return 0;
}
