#ifndef BLITZAR_SDK_SIMULATION_HPP
#define BLITZAR_SDK_SIMULATION_HPP

#include "core/Execution.hpp"
#include "core/Snapshot.hpp"
#include "core/Solver.hpp"
#include "integration/LeapfrogKdk.hpp"
#include "particles/ParticleBuffer.hpp"
#include "solvers/barnes_hut/BarnesHutSolver.hpp"
#include "solvers/direct/DirectSolver.hpp"

#include <blitzar/blitzar.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <variant>
#include <vector>

namespace blitzar_sdk {

class Simulation final {
public:
    explicit Simulation(std::size_t particle_count);

    [[nodiscard]] blitzar_status LastStatus() const noexcept;
    [[nodiscard]] std::size_t ParticleCount() const noexcept;

    [[nodiscard]] blitzar_status SetSolver(
        blitzar_solver_kind solver) noexcept;
    [[nodiscard]] blitzar_status SetIntegrator(
        blitzar_integrator_kind integrator) noexcept;
    [[nodiscard]] blitzar_status SetGravity(
        blitzar_core::Scalar gravitational_constant,
        blitzar_core::Scalar softening) noexcept;
    [[nodiscard]] blitzar_status SetUnits(
        blitzar_core::UnitSystem units) noexcept;
    [[nodiscard]] blitzar_status SetBarnesHut(
        blitzar_core::Scalar opening_angle,
        std::size_t max_particles,
        std::size_t max_cells,
        std::size_t leaf_capacity,
        std::size_t max_depth) noexcept;
    [[nodiscard]] blitzar_status SetTimestep(
        blitzar_core::Scalar timestep) noexcept;
    [[nodiscard]] blitzar_status SetSeed(std::uint64_t seed) noexcept;
    [[nodiscard]] blitzar_status SetParticles(
        std::span<const blitzar_core::Scalar> position_x,
        std::span<const blitzar_core::Scalar> position_y,
        std::span<const blitzar_core::Scalar> position_z,
        std::span<const blitzar_core::Scalar> velocity_x,
        std::span<const blitzar_core::Scalar> velocity_y,
        std::span<const blitzar_core::Scalar> velocity_z,
        std::span<const blitzar_core::Scalar> mass) noexcept;
    [[nodiscard]] blitzar_status GetState(
        std::span<blitzar_core::Scalar> position_x,
        std::span<blitzar_core::Scalar> position_y,
        std::span<blitzar_core::Scalar> position_z,
        std::span<blitzar_core::Scalar> velocity_x,
        std::span<blitzar_core::Scalar> velocity_y,
        std::span<blitzar_core::Scalar> velocity_z,
        std::span<blitzar_core::Scalar> mass) const noexcept;
    [[nodiscard]] blitzar_status Step() noexcept;

private:
    using SolverVariant = std::variant<
        blitzar_direct::DirectSolver,
        blitzar_barnes_hut::BarnesHutSolver>;

    [[nodiscard]] static std::size_t DefaultMaxCells(
        std::size_t particle_count) noexcept;
    [[nodiscard]] static blitzar_status CreateSolver(
        blitzar_solver_kind solver_kind,
        blitzar_physics::GravityParameters gravity,
        blitzar_barnes_hut::BarnesHutSettings barnes_hut,
        SolverVariant& solver) noexcept;
    [[nodiscard]] blitzar_status Remember(blitzar_status status) const noexcept;

    std::size_t particle_count_;
    std::shared_ptr<blitzar_particles::ParticleArena> arena_;
    blitzar_particles::ParticleBuffer particles_;
    blitzar_particles::AccelerationBuffer accelerations_;
    blitzar_integration::LeapfrogWorkspace workspace_;
    blitzar_physics::GravityParameters gravity_;
    blitzar_barnes_hut::BarnesHutSettings barnes_hut_;
    std::vector<std::size_t> traversal_storage_;
    blitzar_solver_kind solver_kind_;
    blitzar_integrator_kind integrator_kind_;
    blitzar_core::Scalar timestep_;
    bool particles_ready_;
    blitzar_core::ExecutionSettings execution_settings_;
    blitzar_core::SnapshotHeader snapshot_header_;
    mutable std::atomic<blitzar_status> last_status_;
    SolverVariant solver_;
    blitzar_integration::LeapfrogKdk integrator_;
};

}  // namespace blitzar_sdk

#endif
