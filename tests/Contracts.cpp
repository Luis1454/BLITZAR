#include "core/Execution.hpp"
#include "core/Snapshot.hpp"
#include "core/Status.hpp"
#include "core/Solver.hpp"
#include "core/Units.hpp"

#include <cassert>
#include <utility>

namespace {

class ProbeSolver final : public blitzar_core::Solver {
public:
    [[nodiscard]] blitzar_core::SolverKind Kind() const noexcept override
    {
        return blitzar_core::SolverKind::Direct;
    }

    [[nodiscard]] blitzar_status Compute(
        blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces,
        const blitzar_core::ExecutionSettings& settings) noexcept override
    {
        if (!blitzar_core::IsValid(particles) ||
            !blitzar_core::IsValid(forces) || particles.count != forces.count ||
            !settings.IsValid()) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        return BLITZAR_STATUS_OK;
    }
};

}  // namespace

int main()
{
    const blitzar_core::ExecutionSettings settings{};
    assert(settings.IsValid());
    assert(blitzar_core::ToPublicStatus(999) == BLITZAR_STATUS_INTERNAL_ERROR);

    blitzar_core::UnitSystem units{};
    assert(units.IsValid());
    units.length_scale = 0.0;
    assert(!units.IsValid());

    blitzar_core::SnapshotHeader snapshot{};
    snapshot.particle_count = 4;
    assert(snapshot.IsCompatible());
    snapshot.magic = 0;
    assert(!snapshot.IsCompatible());

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
    const blitzar_core::ParticleStateView particles{
        1, x, y, z, vx, vy, vz, mass};
    const blitzar_core::ForceView forces{1, fx, fy, fz};
    assert(solver.Kind() == blitzar_core::SolverKind::Direct);
    assert(solver.Compute(particles, forces, settings) == BLITZAR_STATUS_OK);

    const blitzar_core::ParticleStateView invalid{
        1, nullptr, y, z, vx, vy, vz, mass};
    assert(solver.Compute(invalid, forces, settings) ==
           BLITZAR_STATUS_INVALID_ARGUMENT);
    return 0;
}
