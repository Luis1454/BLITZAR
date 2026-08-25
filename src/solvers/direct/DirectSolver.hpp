#ifndef BLITZAR_SOLVERS_DIRECT_DIRECT_SOLVER_HPP
#define BLITZAR_SOLVERS_DIRECT_DIRECT_SOLVER_HPP

#include "core/Execution.hpp"
#include "core/Solver.hpp"
#include "physics/GravityLaw.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <vector>

namespace blitzar_direct {

class DirectSolver final {
public:
    explicit DirectSolver(
        blitzar_physics::GravityParameters parameters, std::size_t staging_capacity = 0);

    [[nodiscard]] blitzar_status Prepare(std::size_t staging_capacity) noexcept;

    [[nodiscard]] blitzar_core::SolverKind Kind() const noexcept;
    [[nodiscard]] blitzar_status Compute(blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings) noexcept;
    [[nodiscard]] blitzar_status ComputeRange(blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings,
        blitzar_core::ForceRange range) noexcept;
    [[nodiscard]] blitzar_status ComputeRemote(blitzar_core::ParticleStateView targets,
        blitzar_core::ParticleStateView sources, blitzar_core::ForceView forces,
        const blitzar_core::ExecutionSettings& settings) noexcept;

private:
    struct ForceTargetRequest final {
        const blitzar_physics::GravityLaw& gravity;
        std::size_t target{};
        blitzar_core::ParticleStateView particles;
        blitzar_core::ForceRange range;
        blitzar_core::Vector3& acceleration;
    };

    struct RemoteForceTargetRequest final {
        const blitzar_physics::GravityLaw& gravity;
        blitzar_core::ParticleStateView targets;
        blitzar_core::ParticleStateView sources;
        std::size_t target{};
        blitzar_core::Vector3& acceleration;
    };

    [[nodiscard]] static bool IsValidState(blitzar_core::ParticleStateView particles) noexcept;
    [[nodiscard]] static blitzar_status CalculateTarget(const ForceTargetRequest& request) noexcept;
    [[nodiscard]] static blitzar_status CalculateRemoteTarget(
        const RemoteForceTargetRequest& request) noexcept;
    [[nodiscard]] bool ValidateRangeRequest(blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings,
        blitzar_core::ForceRange range) const noexcept;
    [[nodiscard]] bool ValidateRemoteRequest(blitzar_core::ParticleStateView targets,
        blitzar_core::ParticleStateView sources, blitzar_core::ForceView forces,
        const blitzar_core::ExecutionSettings& settings) const noexcept;
    [[nodiscard]] blitzar_status ComputeRangeStaged(
        blitzar_core::ParticleStateView particles, blitzar_core::ForceRange range) noexcept;
    [[nodiscard]] blitzar_status CommitRange(
        blitzar_core::ForceView forces, blitzar_core::ForceRange range) noexcept;
    [[nodiscard]] blitzar_status ComputeRemoteStaged(
        blitzar_core::ParticleStateView targets, blitzar_core::ParticleStateView sources) noexcept;
    [[nodiscard]] blitzar_status CommitRemote(blitzar_core::ForceView forces) noexcept;

    blitzar_physics::GravityLaw gravity_;
    std::vector<blitzar_core::Vector3> staging_;
};

} // namespace blitzar_direct

#endif
