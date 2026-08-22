#include "sdk/Simulation.hpp"

#include "particles/ParticleArena.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <new>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace blitzar_sdk {

namespace {

template <typename Solver>
class SolverDispatcher final {
public:
    SolverDispatcher(
        blitzar_gpu::HipContext& hip,
        Solver& cpu,
        blitzar_physics::GravityParameters gravity,
        blitzar_barnes_hut::BarnesHutSettings barnes_hut) noexcept
        : hip_(hip),
          cpu_(cpu),
          gravity_(gravity),
          barnes_hut_(barnes_hut)
    {
    }

    [[nodiscard]] blitzar_status Compute(
        blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces,
        const blitzar_core::ExecutionSettings& settings) noexcept
    {
        if constexpr (std::is_same_v<Solver, blitzar_direct::DirectSolver>) {
            const blitzar_status gpu_status =
                hip_.ComputeDirect(particles, forces, gravity_);
            if (gpu_status == BLITZAR_STATUS_OK) {
                return BLITZAR_STATUS_OK;
            }
        } else {
            const blitzar_status gpu_status = hip_.ComputeBarnesHut(
                particles,
                forces,
                settings,
                gravity_,
                barnes_hut_);
            if (gpu_status == BLITZAR_STATUS_OK) {
                return BLITZAR_STATUS_OK;
            }
        }
        return cpu_.Compute(particles, forces, settings);
    }

    [[nodiscard]] blitzar_status Compute(
        blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces,
        const blitzar_core::ExecutionSettings& settings,
        blitzar_barnes_hut::ThreadWorkspace& workspace) noexcept
    {
        if constexpr (std::is_same_v<Solver, blitzar_direct::DirectSolver>) {
            const blitzar_status gpu_status =
                hip_.ComputeDirect(particles, forces, gravity_);
            if (gpu_status == BLITZAR_STATUS_OK) {
                return BLITZAR_STATUS_OK;
            }
            return cpu_.Compute(particles, forces, settings);
        } else {
            const blitzar_status gpu_status = hip_.ComputeBarnesHut(
                particles,
                forces,
                settings,
                gravity_,
                barnes_hut_);
            if (gpu_status == BLITZAR_STATUS_OK) {
                return BLITZAR_STATUS_OK;
            }
            return cpu_.Compute(particles, forces, settings, workspace);
        }
    }

private:
    blitzar_gpu::HipContext& hip_;
    Solver& cpu_;
    blitzar_physics::GravityParameters gravity_;
    blitzar_barnes_hut::BarnesHutSettings barnes_hut_;
};

[[nodiscard]] blitzar_core::ParticleStateView MakeArenaState(
    blitzar_particles::ParticleArena& arena,
    std::size_t target_count,
    std::size_t source_count) noexcept
{
    if (source_count > arena.Count() || target_count > source_count) {
        return {};
    }
    return {
        target_count,
        arena.PositionX().first(source_count),
        arena.PositionY().first(source_count),
        arena.PositionZ().first(source_count),
        arena.VelocityX().first(source_count),
        arena.VelocityY().first(source_count),
        arena.VelocityZ().first(source_count),
        arena.Mass().first(source_count),
        source_count};
}

[[nodiscard]] blitzar_status AppendGhosts(
    blitzar_parallel::PacketBuffer& ghosts,
    blitzar_particles::ParticleArena& arena,
    std::size_t local_count,
    std::size_t& source_count) noexcept
{
    if (local_count > arena.Count() ||
        ghosts.Size() > arena.Count() - local_count) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    const std::size_t total = local_count + ghosts.Size();
    const auto position_x = arena.PositionX();
    const auto position_y = arena.PositionY();
    const auto position_z = arena.PositionZ();
    const auto velocity_x = arena.VelocityX();
    const auto velocity_y = arena.VelocityY();
    const auto velocity_z = arena.VelocityZ();
    const auto mass = arena.Mass();
    for (std::size_t offset = 0; offset < ghosts.Size(); ++offset) {
        const blitzar_parallel::ParticlePacket& packet = ghosts.View()[offset];
        const std::size_t index = local_count + offset;
        position_x[index] = packet.x;
        position_y[index] = packet.y;
        position_z[index] = packet.z;
        velocity_x[index] = packet.velocity_x;
        velocity_y[index] = packet.velocity_y;
        velocity_z[index] = packet.velocity_z;
        mass[index] = packet.mass;
    }
    source_count = total;
    return BLITZAR_STATUS_OK;
}

[[nodiscard]] blitzar_status StoreLocalPackets(
    blitzar_parallel::PacketBuffer& packets,
    blitzar_particles::ParticleArena& arena,
    blitzar_particles::ParticleBuffer& particles,
    blitzar_particles::AccelerationBuffer& accelerations,
    blitzar_integration::LeapfrogWorkspace& workspace,
    std::span<std::uint64_t> ids,
    std::size_t particle_count,
    std::size_t& local_count) noexcept
{
    if (packets.Size() > arena.Count() || packets.Size() > ids.size()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    for (const blitzar_parallel::ParticlePacket& packet : packets.View()) {
        if (packet.id >= particle_count || !std::isfinite(packet.x) ||
            !std::isfinite(packet.y) || !std::isfinite(packet.z) ||
            !std::isfinite(packet.velocity_x) ||
            !std::isfinite(packet.velocity_y) ||
            !std::isfinite(packet.velocity_z) ||
            !std::isfinite(packet.mass) || packet.mass < 0.0) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }
    const std::size_t count = packets.Size();
    if (particles.SetCount(count) != BLITZAR_STATUS_OK ||
        accelerations.SetCount(count) != BLITZAR_STATUS_OK ||
        workspace.SetCount(count) != BLITZAR_STATUS_OK) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
    const auto position_x = arena.PositionX();
    const auto position_y = arena.PositionY();
    const auto position_z = arena.PositionZ();
    const auto velocity_x = arena.VelocityX();
    const auto velocity_y = arena.VelocityY();
    const auto velocity_z = arena.VelocityZ();
    const auto mass = arena.Mass();
    for (std::size_t index = 0; index < packets.Size(); ++index) {
        const blitzar_parallel::ParticlePacket& packet = packets.View()[index];
        position_x[index] = packet.x;
        position_y[index] = packet.y;
        position_z[index] = packet.z;
        velocity_x[index] = packet.velocity_x;
        velocity_y[index] = packet.velocity_y;
        velocity_z[index] = packet.velocity_z;
        mass[index] = packet.mass;
        ids[index] = packet.id;
    }
    local_count = count;
    return BLITZAR_STATUS_OK;
}

template <typename Solver>
class DistributedSolverDispatcher final {
public:
    DistributedSolverDispatcher(
        blitzar_gpu::HipContext& hip,
        Solver& cpu,
        blitzar_physics::GravityParameters gravity,
        blitzar_barnes_hut::BarnesHutSettings barnes_hut,
        blitzar_parallel::MpiExchange& exchange,
        blitzar_particles::ParticleArena& arena,
        std::span<const std::uint64_t> ids,
        blitzar_parallel::PacketBuffer& ghosts,
        std::size_t& source_count) noexcept
        : base_(hip, cpu, gravity, barnes_hut),
          exchange_(exchange),
          arena_(arena),
          ids_(ids),
          ghosts_(ghosts),
          source_count_(source_count)
    {
    }

    [[nodiscard]] blitzar_status Compute(
        blitzar_core::ParticleStateView local_state,
        blitzar_core::ForceView forces,
        const blitzar_core::ExecutionSettings& settings) noexcept
    {
        const blitzar_status exchange_status = Prepare(local_state);
        if (exchange_status != BLITZAR_STATUS_OK) {
            return exchange_status;
        }
        return base_.Compute(
            MakeArenaState(arena_, local_state.count, source_count_),
            forces,
            settings);
    }

    [[nodiscard]] blitzar_status Compute(
        blitzar_core::ParticleStateView local_state,
        blitzar_core::ForceView forces,
        const blitzar_core::ExecutionSettings& settings,
        blitzar_barnes_hut::ThreadWorkspace& workspace) noexcept
    {
        const blitzar_status exchange_status = Prepare(local_state);
        if (exchange_status != BLITZAR_STATUS_OK) {
            return exchange_status;
        }
        return base_.Compute(
            MakeArenaState(arena_, local_state.count, source_count_),
            forces,
            settings,
            workspace);
    }

private:
    [[nodiscard]] blitzar_status Prepare(
        blitzar_core::ParticleStateView local_state) noexcept
    {
        if (ids_.size() < local_state.count) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        const blitzar_status status = exchange_.ExchangeGhosts(
            local_state,
            ids_.first(local_state.count),
            ghosts_);
        if (status != BLITZAR_STATUS_OK) {
            return status;
        }
        return AppendGhosts(ghosts_, arena_, local_state.count, source_count_);
    }

    SolverDispatcher<Solver> base_;
    blitzar_parallel::MpiExchange& exchange_;
    blitzar_particles::ParticleArena& arena_;
    std::span<const std::uint64_t> ids_;
    blitzar_parallel::PacketBuffer& ghosts_;
    std::size_t& source_count_;
};

}  // namespace

std::size_t Simulation::DefaultMaxCells(std::size_t particle_count) noexcept
{
    if (particle_count == 0) {
        return 1;
    }
    const std::size_t maximum = std::numeric_limits<std::size_t>::max();
    if (particle_count > (maximum - 1) / 8) {
        return 0;
    }
    return particle_count * 8 + 1;
}

Simulation::Simulation(std::size_t particle_count)
    : particle_count_(particle_count),
      mpi_context_(),
      domain_(),
      mpi_exchange_(mpi_context_, domain_),
      hip_context_(),
      arena_(std::make_shared<blitzar_particles::ParticleArena>(particle_count)),
      particles_(arena_),
      accelerations_(arena_),
      workspace_(arena_),
      gravity_{},
      barnes_hut_{
          0.5,
          particle_count == 0 ? 1 : particle_count,
          DefaultMaxCells(particle_count),
          8,
          32},
      traversal_workspace_(barnes_hut_.max_cells, barnes_hut_.max_depth),
      solver_kind_(BLITZAR_SOLVER_DIRECT),
      integrator_kind_(BLITZAR_INTEGRATOR_LEAPFROG_KDK),
      timestep_(1.0),
      particles_ready_(false),
      execution_settings_{},
      snapshot_header_{},
      last_status_(mpi_context_.Status()),
      solver_(std::in_place_type<blitzar_direct::DirectSolver>, gravity_),
      integrator_{},
      particle_ids_(particle_count),
      local_particle_count_(particle_count),
      source_particle_count_(particle_count),
      exchange_buffer_{}
{
    snapshot_header_.particle_count = particle_count_;
}

blitzar_status Simulation::LastStatus() const noexcept
{
    return last_status_.load(std::memory_order_relaxed);
}

std::size_t Simulation::ParticleCount() const noexcept
{
    return particle_count_;
}

blitzar_status Simulation::CreateSolver(
    blitzar_solver_kind solver_kind,
    blitzar_physics::GravityParameters gravity,
    blitzar_barnes_hut::BarnesHutSettings barnes_hut,
    SolverVariant& solver) noexcept
{
    try {
        switch (solver_kind) {
        case BLITZAR_SOLVER_DIRECT:
            solver.emplace<blitzar_direct::DirectSolver>(gravity);
            return BLITZAR_STATUS_OK;
        case BLITZAR_SOLVER_BARNES_HUT:
            if (!barnes_hut.IsValid()) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }
            solver.emplace<blitzar_barnes_hut::BarnesHutSolver>(
                gravity, barnes_hut);
            return BLITZAR_STATUS_OK;
        case BLITZAR_SOLVER_FMM:
        case BLITZAR_SOLVER_PM:
        case BLITZAR_SOLVER_TREEPM:
            return BLITZAR_STATUS_UNSUPPORTED;
        default:
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    } catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    } catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
}

blitzar_status Simulation::Remember(blitzar_status status) const noexcept
{
    last_status_.store(status, std::memory_order_relaxed);
    return status;
}

blitzar_status Simulation::SetSolver(blitzar_solver_kind solver) noexcept
{
    SolverVariant candidate(std::in_place_type<blitzar_direct::DirectSolver>, gravity_);
    const blitzar_status status =
        CreateSolver(solver, gravity_, barnes_hut_, candidate);
    if (status != BLITZAR_STATUS_OK) {
        return Remember(status);
    }
    solver_kind_ = solver;
    solver_ = std::move(candidate);
    return Remember(BLITZAR_STATUS_OK);
}

blitzar_status Simulation::SetIntegrator(blitzar_integrator_kind integrator) noexcept
{
    if (integrator != BLITZAR_INTEGRATOR_LEAPFROG_KDK) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    }
    integrator_kind_ = integrator;
    return Remember(BLITZAR_STATUS_OK);
}

blitzar_status Simulation::SetGravity(
    blitzar_core::Scalar gravitational_constant,
    blitzar_core::Scalar softening) noexcept
{
    const blitzar_physics::GravityParameters candidate_parameters{
        gravitational_constant, softening, gravity_.units};
    if (!candidate_parameters.IsValid()) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    }
    SolverVariant candidate_solver(
        std::in_place_type<blitzar_direct::DirectSolver>, candidate_parameters);
    const blitzar_status status = CreateSolver(
        solver_kind_, candidate_parameters, barnes_hut_, candidate_solver);
    if (status != BLITZAR_STATUS_OK) {
        return Remember(status);
    }
    gravity_ = candidate_parameters;
    solver_ = std::move(candidate_solver);
    return Remember(BLITZAR_STATUS_OK);
}

blitzar_status Simulation::SetUnits(blitzar_core::UnitSystem units) noexcept
{
    if (!units.IsValid()) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    }
    const blitzar_physics::GravityParameters candidate_parameters{
        gravity_.gravitational_constant, gravity_.softening, units};
    if (!candidate_parameters.IsValid()) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    }
    SolverVariant candidate_solver(
        std::in_place_type<blitzar_direct::DirectSolver>, candidate_parameters);
    const blitzar_status status = CreateSolver(
        solver_kind_, candidate_parameters, barnes_hut_, candidate_solver);
    if (status != BLITZAR_STATUS_OK) {
        return Remember(status);
    }
    gravity_ = candidate_parameters;
    solver_ = std::move(candidate_solver);
    return Remember(BLITZAR_STATUS_OK);
}

blitzar_status Simulation::SetBarnesHut(
    blitzar_core::Scalar opening_angle,
    std::size_t max_particles,
    std::size_t max_cells,
    std::size_t leaf_capacity,
    std::size_t max_depth) noexcept
{
    const blitzar_barnes_hut::BarnesHutSettings candidate_settings{
        opening_angle, max_particles, max_cells, leaf_capacity, max_depth};
    if (!candidate_settings.IsValid() || max_particles < particle_count_) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    }
    blitzar_barnes_hut::ThreadWorkspace candidate_workspace(0, 0);
    try {
        candidate_workspace = blitzar_barnes_hut::ThreadWorkspace(
            candidate_settings.max_cells, candidate_settings.max_depth);
    } catch (const std::length_error&) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    } catch (const std::bad_alloc&) {
        return Remember(BLITZAR_STATUS_ALLOCATION_FAILURE);
    }
    if (solver_kind_ == BLITZAR_SOLVER_BARNES_HUT) {
        SolverVariant candidate_solver(
            std::in_place_type<blitzar_direct::DirectSolver>, gravity_);
        const blitzar_status status = CreateSolver(
            solver_kind_, gravity_, candidate_settings, candidate_solver);
        if (status != BLITZAR_STATUS_OK) {
            return Remember(status);
        }
        solver_ = std::move(candidate_solver);
    }
    traversal_workspace_ = std::move(candidate_workspace);
    barnes_hut_ = candidate_settings;
    return Remember(BLITZAR_STATUS_OK);
}

blitzar_status Simulation::SetTimestep(blitzar_core::Scalar timestep) noexcept
{
    if (!std::isfinite(timestep) || timestep <= 0.0) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    }
    timestep_ = timestep;
    return Remember(BLITZAR_STATUS_OK);
}

blitzar_status Simulation::SetSeed(std::uint64_t seed) noexcept
{
    execution_settings_.seed = seed;
    return Remember(BLITZAR_STATUS_OK);
}

blitzar_status Simulation::SetParticles(
    std::span<const blitzar_core::Scalar> position_x,
    std::span<const blitzar_core::Scalar> position_y,
    std::span<const blitzar_core::Scalar> position_z,
    std::span<const blitzar_core::Scalar> velocity_x,
    std::span<const blitzar_core::Scalar> velocity_y,
    std::span<const blitzar_core::Scalar> velocity_z,
    std::span<const blitzar_core::Scalar> mass) noexcept
{
    if (!mpi_context_.IsUsable()) {
        return Remember(mpi_context_.Status());
    }
    if (position_x.size() != particle_count_ ||
        position_y.size() != particle_count_ ||
        position_z.size() != particle_count_ ||
        velocity_x.size() != particle_count_ ||
        velocity_y.size() != particle_count_ ||
        velocity_z.size() != particle_count_ || mass.size() != particle_count_) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    }
    if (particles_.SetCount(particle_count_) != BLITZAR_STATUS_OK ||
        accelerations_.SetCount(particle_count_) != BLITZAR_STATUS_OK ||
        workspace_.SetCount(particle_count_) != BLITZAR_STATUS_OK) {
        return Remember(BLITZAR_STATUS_INTERNAL_ERROR);
    }
    for (std::size_t index = 0; index < particle_count_; ++index) {
        if (!std::isfinite(position_x[index]) ||
            !std::isfinite(position_y[index]) ||
            !std::isfinite(position_z[index]) ||
            !std::isfinite(velocity_x[index]) ||
            !std::isfinite(velocity_y[index]) ||
            !std::isfinite(velocity_z[index]) || !std::isfinite(mass[index]) ||
            mass[index] < 0.0) {
            return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
        }
    }
    for (std::size_t index = 0; index < particle_count_; ++index) {
        if (particles_.SetPosition(
                index, {position_x[index], position_y[index], position_z[index]}) !=
                BLITZAR_STATUS_OK ||
            particles_.SetVelocity(
                index,
                {velocity_x[index], velocity_y[index], velocity_z[index]}) !=
                BLITZAR_STATUS_OK ||
            particles_.SetMass(index, mass[index]) != BLITZAR_STATUS_OK) {
            return Remember(BLITZAR_STATUS_INTERNAL_ERROR);
        }
        particle_ids_[index] = static_cast<std::uint64_t>(index);
    }
    const blitzar_status domain_status =
        domain_.Initialize(particles_.State(), mpi_context_);
    if (domain_status != BLITZAR_STATUS_OK) {
        return Remember(domain_status);
    }
    std::vector<std::size_t> local_indices;
    const blitzar_status index_status =
        domain_.LocalIndices(particles_.State(), local_indices);
    if (index_status != BLITZAR_STATUS_OK) {
        return Remember(index_status);
    }
    const auto local_position_x = particles_.MutableView().x;
    const auto local_position_y = particles_.MutableView().y;
    const auto local_position_z = particles_.MutableView().z;
    const auto local_velocity_x = particles_.MutableView().velocity_x;
    const auto local_velocity_y = particles_.MutableView().velocity_y;
    const auto local_velocity_z = particles_.MutableView().velocity_z;
    const auto local_mass = arena_->Mass();
    for (std::size_t local = 0; local < local_indices.size(); ++local) {
        const std::size_t global = local_indices[local];
        local_position_x[local] = position_x[global];
        local_position_y[local] = position_y[global];
        local_position_z[local] = position_z[global];
        local_velocity_x[local] = velocity_x[global];
        local_velocity_y[local] = velocity_y[global];
        local_velocity_z[local] = velocity_z[global];
        local_mass[local] = mass[global];
        particle_ids_[local] = static_cast<std::uint64_t>(global);
    }
    if (particles_.SetCount(local_indices.size()) != BLITZAR_STATUS_OK ||
        accelerations_.SetCount(local_indices.size()) != BLITZAR_STATUS_OK ||
        workspace_.SetCount(local_indices.size()) != BLITZAR_STATUS_OK) {
        return Remember(BLITZAR_STATUS_INTERNAL_ERROR);
    }
    local_particle_count_ = local_indices.size();
    source_particle_count_ = local_particle_count_;
    particles_ready_ = true;
    return Remember(BLITZAR_STATUS_OK);
}

blitzar_status Simulation::GetState(
    std::span<blitzar_core::Scalar> position_x,
    std::span<blitzar_core::Scalar> position_y,
    std::span<blitzar_core::Scalar> position_z,
    std::span<blitzar_core::Scalar> velocity_x,
    std::span<blitzar_core::Scalar> velocity_y,
    std::span<blitzar_core::Scalar> velocity_z,
    std::span<blitzar_core::Scalar> mass) const noexcept
{
    if (!particles_ready_ || position_x.size() < particle_count_ ||
        position_y.size() < particle_count_ ||
        position_z.size() < particle_count_ ||
        velocity_x.size() < particle_count_ ||
        velocity_y.size() < particle_count_ ||
        velocity_z.size() < particle_count_ || mass.size() < particle_count_) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    }
    if (!mpi_context_.IsDistributed()) {
        const blitzar_core::ParticleStateView state = particles_.State();
        std::copy_n(state.x.begin(), particle_count_, position_x.begin());
        std::copy_n(state.y.begin(), particle_count_, position_y.begin());
        std::copy_n(state.z.begin(), particle_count_, position_z.begin());
        std::copy_n(state.velocity_x.begin(), particle_count_, velocity_x.begin());
        std::copy_n(state.velocity_y.begin(), particle_count_, velocity_y.begin());
        std::copy_n(state.velocity_z.begin(), particle_count_, velocity_z.begin());
        std::copy_n(state.mass.begin(), particle_count_, mass.begin());
        return Remember(BLITZAR_STATUS_OK);
    }

    blitzar_parallel::PacketBuffer gathered;
    const blitzar_status gather_status = mpi_exchange_.Gather(
        particles_.State(),
        std::span<const std::uint64_t>(particle_ids_).first(local_particle_count_),
        gathered);
    if (gather_status != BLITZAR_STATUS_OK) {
        return Remember(gather_status);
    }
    if (gathered.Size() != particle_count_) {
        return Remember(BLITZAR_STATUS_INTERNAL_ERROR);
    }
    std::vector<unsigned char> seen;
    try {
        seen.assign(particle_count_, 0);
    } catch (const std::bad_alloc&) {
        return Remember(BLITZAR_STATUS_ALLOCATION_FAILURE);
    }
    for (const blitzar_parallel::ParticlePacket& packet : gathered.View()) {
        if (packet.id >= particle_count_ || seen[packet.id] != 0 ||
            !std::isfinite(packet.x) || !std::isfinite(packet.y) ||
            !std::isfinite(packet.z) || !std::isfinite(packet.velocity_x) ||
            !std::isfinite(packet.velocity_y) ||
            !std::isfinite(packet.velocity_z) || !std::isfinite(packet.mass) ||
            packet.mass < 0.0) {
            return Remember(BLITZAR_STATUS_INTERNAL_ERROR);
        }
        seen[packet.id] = 1;
        position_x[packet.id] = packet.x;
        position_y[packet.id] = packet.y;
        position_z[packet.id] = packet.z;
        velocity_x[packet.id] = packet.velocity_x;
        velocity_y[packet.id] = packet.velocity_y;
        velocity_z[packet.id] = packet.velocity_z;
        mass[packet.id] = packet.mass;
    }
    if (std::find(seen.begin(), seen.end(), 0) != seen.end()) {
        return Remember(BLITZAR_STATUS_INTERNAL_ERROR);
    }
    return Remember(BLITZAR_STATUS_OK);
}

blitzar_status Simulation::Step() noexcept
{
    if (!particles_ready_ || integrator_kind_ != BLITZAR_INTEGRATOR_LEAPFROG_KDK ||
        !std::isfinite(timestep_) || timestep_ <= 0.0) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    }
    blitzar_status status = std::visit(
        [this](auto& solver) {
            if (mpi_context_.IsDistributed()) {
                DistributedSolverDispatcher dispatcher(
                    hip_context_,
                    solver,
                    gravity_,
                    barnes_hut_,
                    mpi_exchange_,
                    *arena_,
                    std::span<const std::uint64_t>(particle_ids_),
                    exchange_buffer_,
                    source_particle_count_);
                return integrator_.Advance(
                    particles_,
                    accelerations_,
                    workspace_,
                    dispatcher,
                    timestep_,
                    execution_settings_,
                    traversal_workspace_,
                    particles_.State());
            }
            SolverDispatcher dispatcher(
                hip_context_, solver, gravity_, barnes_hut_);
            return integrator_.Advance(
                particles_,
                accelerations_,
                workspace_,
                dispatcher,
                timestep_,
                execution_settings_,
                traversal_workspace_);
        },
        solver_);
    if (status == BLITZAR_STATUS_OK && mpi_context_.IsDistributed()) {
        blitzar_parallel::PacketBuffer migrated;
        status = mpi_exchange_.Migrate(
            particles_.State(),
            std::span<const std::uint64_t>(particle_ids_).first(local_particle_count_),
            migrated);
        if (status == BLITZAR_STATUS_OK) {
            status = StoreLocalPackets(
                migrated,
                *arena_,
                particles_,
                accelerations_,
                workspace_,
                std::span<std::uint64_t>(particle_ids_),
                particle_count_,
                local_particle_count_);
            source_particle_count_ = local_particle_count_;
        }
    }
    return Remember(status);
}

}  // namespace blitzar_sdk
