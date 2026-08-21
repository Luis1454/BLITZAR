#ifndef BLITZAR_CORE_SOLVER_HPP
#define BLITZAR_CORE_SOLVER_HPP

#include "Execution.hpp"
#include "Types.hpp"

#include <blitzar/blitzar.h>

#include <cstdint>

namespace blitzar_core {

enum class SolverKind : std::uint8_t {
    Direct = 0,
    BarnesHut = 1,
    Fmm = 2,
    Pm = 3,
    TreePm = 4
};

class Solver {
public:
    virtual ~Solver() = default;

    [[nodiscard]] virtual SolverKind Kind() const noexcept = 0;

    [[nodiscard]] virtual blitzar_status Compute(
        ParticleStateView particles,
        ForceView forces,
        const ExecutionSettings& settings) noexcept = 0;
};

}  // namespace blitzar_core

#endif
