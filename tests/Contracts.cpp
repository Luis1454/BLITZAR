#include "core/Execution.hpp"
#include "core/Snapshot.hpp"
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
        blitzar_core::ParticleView particles,
        blitzar_core::ForceView forces,
        const blitzar_core::ExecutionSettings& settings) const noexcept override
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
    blitzar_core::Scalar fx[1]{};
    blitzar_core::Scalar fy[1]{};
    blitzar_core::Scalar fz[1]{};
    const blitzar_core::ParticleView particles{1, x, y, z};
    const blitzar_core::ForceView forces{1, fx, fy, fz};
    assert(solver.Kind() == blitzar_core::SolverKind::Direct);
    assert(solver.Compute(particles, forces, settings) == BLITZAR_STATUS_OK);

    const blitzar_core::ParticleView invalid{1, nullptr, y, z};
    assert(solver.Compute(invalid, forces, settings) ==
           BLITZAR_STATUS_INVALID_ARGUMENT);
    return 0;
}
