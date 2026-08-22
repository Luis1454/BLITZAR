#include "sdk/Simulation.hpp"

#include "particles/ParticleArena.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
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

    [[nodiscard]] blitzar_status ComputeRange(
        blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces,
        const blitzar_core::ExecutionSettings& settings,
        std::size_t source_begin,
        std::size_t source_end,
        bool accumulate) noexcept
    {
        if constexpr (std::is_same_v<Solver, blitzar_direct::DirectSolver>) {
            const blitzar_status gpu_status = hip_.ComputeDirectRange(
                particles,
                forces,
                gravity_,
                source_begin,
                source_end,
                accumulate);
            if (gpu_status == BLITZAR_STATUS_OK) {
                return BLITZAR_STATUS_OK;
            }
            return cpu_.ComputeRange(
                particles,
                forces,
                settings,
                source_begin,
                source_end,
                accumulate);
        } else {
            return BLITZAR_STATUS_UNSUPPORTED;
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

struct ParticleInputStage final {
    std::vector<blitzar_core::Scalar> position_x;
    std::vector<blitzar_core::Scalar> position_y;
    std::vector<blitzar_core::Scalar> position_z;
    std::vector<blitzar_core::Scalar> velocity_x;
    std::vector<blitzar_core::Scalar> velocity_y;
    std::vector<blitzar_core::Scalar> velocity_z;
    std::vector<blitzar_core::Scalar> mass;

    [[nodiscard]] blitzar_core::ParticleStateView State() const noexcept
    {
        return {
            position_x.size(),
            std::span<const blitzar_core::Scalar>(position_x),
            std::span<const blitzar_core::Scalar>(position_y),
            std::span<const blitzar_core::Scalar>(position_z),
            std::span<const blitzar_core::Scalar>(velocity_x),
            std::span<const blitzar_core::Scalar>(velocity_y),
            std::span<const blitzar_core::Scalar>(velocity_z),
            std::span<const blitzar_core::Scalar>(mass),
            position_x.size()};
    }
};

[[nodiscard]] blitzar_status StageParticleInput(
    std::span<const blitzar_core::Scalar> position_x,
    std::span<const blitzar_core::Scalar> position_y,
    std::span<const blitzar_core::Scalar> position_z,
    std::span<const blitzar_core::Scalar> velocity_x,
    std::span<const blitzar_core::Scalar> velocity_y,
    std::span<const blitzar_core::Scalar> velocity_z,
    std::span<const blitzar_core::Scalar> mass,
    ParticleInputStage& stage) noexcept
{
    try {
        stage.position_x.resize(position_x.size());
        stage.position_y.resize(position_y.size());
        stage.position_z.resize(position_z.size());
        stage.velocity_x.resize(velocity_x.size());
        stage.velocity_y.resize(velocity_y.size());
        stage.velocity_z.resize(velocity_z.size());
        stage.mass.resize(mass.size());
    } catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    } catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    for (std::size_t index = 0; index < position_x.size(); ++index) {
        if (!std::isfinite(position_x[index]) ||
            !std::isfinite(position_y[index]) ||
            !std::isfinite(position_z[index]) ||
            !std::isfinite(velocity_x[index]) ||
            !std::isfinite(velocity_y[index]) ||
            !std::isfinite(velocity_z[index]) || !std::isfinite(mass[index]) ||
            mass[index] < 0.0) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        stage.position_x[index] = position_x[index];
        stage.position_y[index] = position_y[index];
        stage.position_z[index] = position_z[index];
        stage.velocity_x[index] = velocity_x[index];
        stage.velocity_y[index] = velocity_y[index];
        stage.velocity_z[index] = velocity_z[index];
        stage.mass[index] = mass[index];
    }
    return BLITZAR_STATUS_OK;
}

[[nodiscard]] blitzar_status SynchronizeSimulationStatus(
    const blitzar_parallel::MpiContext& context,
    blitzar_status local_status,
    const char* phase) noexcept
{
    if (!context.IsDistributed()) {
        return local_status;
    }
    blitzar_status global_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status = context.SynchronizeStatus(
        local_status,
        "Simulation",
        phase,
        global_status);
    return synchronization_status == BLITZAR_STATUS_OK
               ? global_status
               : synchronization_status;
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
    const std::size_t previous_particle_count = particles.Count();
    const std::size_t previous_acceleration_count = accelerations.Count();
    const std::size_t previous_workspace_count = workspace.Count();
    if (particles.SetCount(count) != BLITZAR_STATUS_OK) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
    if (accelerations.SetCount(count) != BLITZAR_STATUS_OK) {
        (void)particles.SetCount(previous_particle_count);
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
    if (workspace.SetCount(count) != BLITZAR_STATUS_OK) {
        (void)particles.SetCount(previous_particle_count);
        (void)accelerations.SetCount(previous_acceleration_count);
        (void)workspace.SetCount(previous_workspace_count);
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

[[nodiscard]] blitzar_status CopyPacketBuffer(
    const blitzar_parallel::PacketBuffer& source,
    blitzar_parallel::PacketBuffer& target) noexcept
{
    try {
        target.Resize(source.Size());
        std::copy(source.View().begin(), source.View().end(), target.View().begin());
    } catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    } catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    return BLITZAR_STATUS_OK;
}

[[nodiscard]] blitzar_status CaptureArenaState(
    blitzar_particles::ParticleArena& arena,
    std::size_t local_count,
    std::size_t source_count,
    std::span<const std::uint64_t> ids,
    blitzar_parallel::PacketBuffer& snapshot) noexcept
{
    if (local_count > source_count || source_count > arena.Count() ||
        local_count > ids.size()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    try {
        snapshot.Resize(source_count);
    } catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    } catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    const auto position_x = arena.PositionX();
    const auto position_y = arena.PositionY();
    const auto position_z = arena.PositionZ();
    const auto velocity_x = arena.VelocityX();
    const auto velocity_y = arena.VelocityY();
    const auto velocity_z = arena.VelocityZ();
    const auto mass = arena.Mass();
    for (std::size_t index = 0; index < source_count; ++index) {
        snapshot.View()[index] = {
            index < local_count ? ids[index] : 0,
            position_x[index],
            position_y[index],
            position_z[index],
            velocity_x[index],
            velocity_y[index],
            velocity_z[index],
            mass[index]};
    }
    return BLITZAR_STATUS_OK;
}

[[nodiscard]] blitzar_status RestoreArenaState(
    const blitzar_parallel::PacketBuffer& snapshot,
    blitzar_particles::ParticleArena& arena,
    blitzar_particles::ParticleBuffer& particles,
    std::span<std::uint64_t> ids,
    std::size_t local_count,
    std::size_t source_count) noexcept
{
    if (snapshot.Size() != source_count || local_count > source_count ||
        source_count > arena.Count() || local_count > ids.size()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
    if (particles.SetCount(local_count) != BLITZAR_STATUS_OK) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
    const auto position_x = arena.PositionX();
    const auto position_y = arena.PositionY();
    const auto position_z = arena.PositionZ();
    const auto velocity_x = arena.VelocityX();
    const auto velocity_y = arena.VelocityY();
    const auto velocity_z = arena.VelocityZ();
    const auto mass = arena.Mass();
    for (std::size_t index = 0; index < source_count; ++index) {
        const blitzar_parallel::ParticlePacket& packet = snapshot.View()[index];
        position_x[index] = packet.x;
        position_y[index] = packet.y;
        position_z[index] = packet.z;
        velocity_x[index] = packet.velocity_x;
        velocity_y[index] = packet.velocity_y;
        velocity_z[index] = packet.velocity_z;
        mass[index] = packet.mass;
        if (index < local_count) {
            ids[index] = packet.id;
        }
    }
    return BLITZAR_STATUS_OK;
}

[[nodiscard]] blitzar_status CaptureForceState(
    blitzar_core::ForceView force,
    blitzar_parallel::PacketBuffer& snapshot) noexcept
{
    if (!blitzar_core::IsValid(force)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    try {
        snapshot.Resize(force.count);
    } catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    } catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    for (std::size_t index = 0; index < force.count; ++index) {
        snapshot.View()[index] = {
            0,
            force.x[index],
            force.y[index],
            force.z[index],
            0.0,
            0.0,
            0.0,
            0.0};
    }
    return BLITZAR_STATUS_OK;
}

[[nodiscard]] blitzar_status RestoreForceState(
    const blitzar_parallel::PacketBuffer& snapshot,
    blitzar_core::ForceView force) noexcept
{
    if (!blitzar_core::IsValid(force) || snapshot.Size() != force.count) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
    for (std::size_t index = 0; index < force.count; ++index) {
        const blitzar_parallel::ParticlePacket& packet = snapshot.View()[index];
        force.x[index] = packet.x;
        force.y[index] = packet.y;
        force.z[index] = packet.z;
    }
    return BLITZAR_STATUS_OK;
}

class DistributedStepTransaction final {
public:
    DistributedStepTransaction(
        blitzar_particles::ParticleArena& arena,
        blitzar_particles::ParticleBuffer& particles,
        blitzar_particles::AccelerationBuffer& accelerations,
        blitzar_integration::LeapfrogWorkspace& workspace,
        std::span<std::uint64_t> ids,
        std::size_t& local_count,
        std::size_t& source_count,
        blitzar_parallel::PacketBuffer& exchange,
        blitzar_parallel::PacketBuffer& arena_snapshot,
        blitzar_parallel::PacketBuffer& force_snapshot,
        blitzar_parallel::PacketBuffer& exchange_snapshot) noexcept
        : arena_(arena),
          particles_(particles),
          accelerations_(accelerations),
          workspace_(workspace),
          ids_(ids),
          local_count_(local_count),
          source_count_(source_count),
          exchange_(exchange),
          arena_snapshot_(arena_snapshot),
          force_snapshot_(force_snapshot),
          exchange_snapshot_(exchange_snapshot)
    {
    }

    [[nodiscard]] blitzar_status Prepare() noexcept
    {
        phase_ = Phase::Aborted;
        arena_snapshot_.Clear();
        force_snapshot_.Clear();
        exchange_snapshot_.Clear();
        local_count_before_ = particles_.Count();
        source_count_before_ = source_count_;
        acceleration_count_before_ = accelerations_.Count();
        workspace_count_before_ = workspace_.Count();
        if (!arena_.IsValid() || !particles_.IsValid() ||
            !accelerations_.IsValid() || !workspace_.IsValid() ||
            local_count_before_ != local_count_ ||
            local_count_before_ != acceleration_count_before_ ||
            local_count_before_ != workspace_count_before_ ||
            local_count_before_ > source_count_before_ ||
            source_count_before_ > arena_.Count() ||
            local_count_before_ > ids_.size()) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        blitzar_status status = CaptureArenaState(
            arena_,
            local_count_before_,
            source_count_before_,
            ids_,
            arena_snapshot_);
        if (status != BLITZAR_STATUS_OK) {
            ResetSnapshots();
            return status;
        }
        status = CaptureForceState(accelerations_.View(), force_snapshot_);
        if (status != BLITZAR_STATUS_OK) {
            ResetSnapshots();
            return status;
        }
        status = CopyPacketBuffer(exchange_, exchange_snapshot_);
        if (status != BLITZAR_STATUS_OK) {
            ResetSnapshots();
            return status;
        }
        phase_ = Phase::Prepared;
        return BLITZAR_STATUS_OK;
    }

    void Begin() noexcept
    {
        if (phase_ == Phase::Prepared) {
            phase_ = Phase::InFlight;
        }
    }

    void Complete() noexcept
    {
        if (phase_ == Phase::InFlight) {
            phase_ = Phase::Complete;
        }
    }

    void Commit() noexcept
    {
        if (phase_ == Phase::Complete) {
            ResetSnapshots();
            phase_ = Phase::Committed;
        }
    }

    void Abort() noexcept
    {
        if (phase_ == Phase::Committed || phase_ == Phase::Aborted) {
            return;
        }
        (void)RestoreArenaState(
            arena_snapshot_,
            arena_,
            particles_,
            ids_,
            local_count_before_,
            source_count_before_);
        (void)accelerations_.SetCount(acceleration_count_before_);
        (void)workspace_.SetCount(workspace_count_before_);
        (void)RestoreForceState(force_snapshot_, accelerations_.View());
        local_count_ = local_count_before_;
        source_count_ = source_count_before_;
        (void)workspace_.Capture(particles_.MutableView());
        (void)CopyPacketBuffer(exchange_snapshot_, exchange_);
        ResetSnapshots();
        phase_ = Phase::Aborted;
    }

private:
    enum class Phase : std::uint8_t {
        Aborted,
        Prepared,
        InFlight,
        Complete,
        Committed,
    };

    void ResetSnapshots() noexcept
    {
        arena_snapshot_.Clear();
        force_snapshot_.Clear();
        exchange_snapshot_.Clear();
    }

    blitzar_particles::ParticleArena& arena_;
    blitzar_particles::ParticleBuffer& particles_;
    blitzar_particles::AccelerationBuffer& accelerations_;
    blitzar_integration::LeapfrogWorkspace& workspace_;
    std::span<std::uint64_t> ids_;
    std::size_t& local_count_;
    std::size_t& source_count_;
    blitzar_parallel::PacketBuffer& exchange_;
    blitzar_parallel::PacketBuffer& arena_snapshot_;
    blitzar_parallel::PacketBuffer& force_snapshot_;
    blitzar_parallel::PacketBuffer& exchange_snapshot_;
    std::size_t local_count_before_{0};
    std::size_t source_count_before_{0};
    std::size_t acceleration_count_before_{0};
    std::size_t workspace_count_before_{0};
    Phase phase_{Phase::Aborted};
};

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
        return ComputeWithOverlap(local_state, forces, settings, nullptr);
    }

    [[nodiscard]] blitzar_status Compute(
        blitzar_core::ParticleStateView local_state,
        blitzar_core::ForceView forces,
        const blitzar_core::ExecutionSettings& settings,
        blitzar_barnes_hut::ThreadWorkspace& workspace) noexcept
    {
        return ComputeWithOverlap(local_state, forces, settings, &workspace);
    }

    void Abort() noexcept
    {
        exchange_.AbortGhosts(halo_, ghosts_);
    }

private:
    [[nodiscard]] blitzar_status ComputeWithOverlap(
        blitzar_core::ParticleStateView local_state,
        blitzar_core::ForceView forces,
        const blitzar_core::ExecutionSettings& settings,
        blitzar_barnes_hut::ThreadWorkspace* workspace) noexcept
    {
        const bool ids_valid = ids_.size() >= local_state.count;
        const std::span<const std::uint64_t> local_ids = ids_valid
                                                              ? ids_.first(local_state.count)
                                                              : std::span<const std::uint64_t>{};
        const blitzar_status begin_status = exchange_.BeginGhosts(
            local_state,
            local_ids,
            halo_);
        if (begin_status != BLITZAR_STATUS_OK) {
            return begin_status;
        }

        blitzar_status local_status = BLITZAR_STATUS_OK;
        if constexpr (std::is_same_v<Solver, blitzar_direct::DirectSolver>) {
            local_status = base_.ComputeRange(
                local_state,
                forces,
                settings,
                0,
                local_state.count,
                false);
        } else if (workspace != nullptr) {
            local_status = base_.Compute(
                local_state,
                forces,
                settings,
                *workspace);
        } else {
            local_status = base_.Compute(local_state, forces, settings);
        }

        const blitzar_status complete_status =
            exchange_.CompleteGhosts(halo_, ghosts_);
        if (complete_status != BLITZAR_STATUS_OK) {
            return complete_status;
        }
        const blitzar_status synchronized_local_status =
            exchange_.SynchronizeStatus(local_status, "force-local");
        if (synchronized_local_status != BLITZAR_STATUS_OK) {
            return synchronized_local_status;
        }
        const blitzar_status append_status =
            AppendGhosts(ghosts_, arena_, local_state.count, source_count_);
        const blitzar_status synchronized_append_status =
            exchange_.SynchronizeStatus(append_status, "force-append");
        if (synchronized_append_status != BLITZAR_STATUS_OK) {
            return synchronized_append_status;
        }

        const blitzar_core::ParticleStateView full_state = MakeArenaState(
            arena_,
            local_state.count,
            source_count_);
        blitzar_status remote_status = BLITZAR_STATUS_OK;
        if constexpr (std::is_same_v<Solver, blitzar_direct::DirectSolver>) {
            if (source_count_ != local_state.count) {
                remote_status = base_.ComputeRange(
                    full_state,
                    forces,
                    settings,
                    local_state.count,
                    source_count_,
                    true);
            }
        } else if (workspace != nullptr) {
            remote_status = base_.Compute(full_state, forces, settings, *workspace);
        } else {
            remote_status = base_.Compute(full_state, forces, settings);
        }
        return exchange_.SynchronizeStatus(remote_status, "force-remote");
    }

    SolverDispatcher<Solver> base_;
    blitzar_parallel::MpiExchange& exchange_;
    blitzar_particles::ParticleArena& arena_;
    std::span<const std::uint64_t> ids_;
    blitzar_parallel::PacketBuffer& ghosts_;
    std::size_t& source_count_;
    blitzar_parallel::MpiContext::GhostExchange halo_;
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
      exchange_buffer_{},
      rollback_arena_buffer_{},
      rollback_force_buffer_{},
      rollback_exchange_buffer_{}
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
    const bool input_sizes_valid =
        position_x.size() == particle_count_ &&
        position_y.size() == particle_count_ &&
        position_z.size() == particle_count_ &&
        velocity_x.size() == particle_count_ &&
        velocity_y.size() == particle_count_ &&
        velocity_z.size() == particle_count_ && mass.size() == particle_count_;
    blitzar_status input_status = SynchronizeSimulationStatus(
        mpi_context_,
        input_sizes_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT,
        "set-particles-input");
    if (input_status != BLITZAR_STATUS_OK) {
        return Remember(input_status);
    }

    ParticleInputStage stage;
    input_status = StageParticleInput(
        position_x,
        position_y,
        position_z,
        velocity_x,
        velocity_y,
        velocity_z,
        mass,
        stage);
    input_status = SynchronizeSimulationStatus(
        mpi_context_, input_status, "set-particles-stage");
    if (input_status != BLITZAR_STATUS_OK) {
        return Remember(input_status);
    }

    blitzar_parallel::DomainDecomposition candidate_domain;
    blitzar_status domain_status =
        candidate_domain.Initialize(stage.State(), mpi_context_);
    domain_status = SynchronizeSimulationStatus(
        mpi_context_, domain_status, "set-particles-domain");
    if (domain_status != BLITZAR_STATUS_OK) {
        return Remember(domain_status);
    }
    std::vector<std::size_t> local_indices;
    blitzar_status index_status =
        candidate_domain.LocalIndices(stage.State(), local_indices);
    index_status = SynchronizeSimulationStatus(
        mpi_context_, index_status, "set-particles-indices");
    if (index_status != BLITZAR_STATUS_OK) {
        return Remember(index_status);
    }

    const std::size_t local_count = local_indices.size();
    const blitzar_status capacity_status =
        local_count <= arena_->Count() && local_count <= particle_ids_.size()
            ? BLITZAR_STATUS_OK
            : BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronized_capacity_status =
        SynchronizeSimulationStatus(
            mpi_context_, capacity_status, "set-particles-capacity");
    if (synchronized_capacity_status != BLITZAR_STATUS_OK) {
        return Remember(synchronized_capacity_status);
    }

    const std::size_t previous_particle_count = particles_.Count();
    const std::size_t previous_acceleration_count = accelerations_.Count();
    const std::size_t previous_workspace_count = workspace_.Count();
    blitzar_status commit_status = BLITZAR_STATUS_OK;
    if (particles_.SetCount(local_count) != BLITZAR_STATUS_OK) {
        commit_status = BLITZAR_STATUS_INTERNAL_ERROR;
    }
    if (commit_status == BLITZAR_STATUS_OK &&
        accelerations_.SetCount(local_count) != BLITZAR_STATUS_OK) {
        commit_status = BLITZAR_STATUS_INTERNAL_ERROR;
    }
    if (commit_status == BLITZAR_STATUS_OK &&
        workspace_.SetCount(local_count) != BLITZAR_STATUS_OK) {
        commit_status = BLITZAR_STATUS_INTERNAL_ERROR;
    }
    if (commit_status != BLITZAR_STATUS_OK) {
        (void)particles_.SetCount(previous_particle_count);
        (void)accelerations_.SetCount(previous_acceleration_count);
        (void)workspace_.SetCount(previous_workspace_count);
    }
    commit_status = SynchronizeSimulationStatus(
        mpi_context_, commit_status, "set-particles-commit");
    if (commit_status != BLITZAR_STATUS_OK) {
        (void)particles_.SetCount(previous_particle_count);
        (void)accelerations_.SetCount(previous_acceleration_count);
        (void)workspace_.SetCount(previous_workspace_count);
        return Remember(commit_status);
    }

    const auto local_position_x = arena_->PositionX();
    const auto local_position_y = arena_->PositionY();
    const auto local_position_z = arena_->PositionZ();
    const auto local_velocity_x = arena_->VelocityX();
    const auto local_velocity_y = arena_->VelocityY();
    const auto local_velocity_z = arena_->VelocityZ();
    const auto local_mass = arena_->Mass();
    for (std::size_t local = 0; local < local_count; ++local) {
        const std::size_t global = local_indices[local];
        local_position_x[local] = stage.position_x[global];
        local_position_y[local] = stage.position_y[global];
        local_position_z[local] = stage.position_z[global];
        local_velocity_x[local] = stage.velocity_x[global];
        local_velocity_y[local] = stage.velocity_y[global];
        local_velocity_z[local] = stage.velocity_z[global];
        local_mass[local] = stage.mass[global];
        particle_ids_[local] = static_cast<std::uint64_t>(global);
    }
    domain_ = std::move(candidate_domain);
    local_particle_count_ = local_count;
    source_particle_count_ = local_particle_count_;
    exchange_buffer_.Clear();
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
    const bool output_valid =
        particles_ready_ && position_x.size() >= particle_count_ &&
        position_y.size() >= particle_count_ &&
        position_z.size() >= particle_count_ &&
        velocity_x.size() >= particle_count_ &&
        velocity_y.size() >= particle_count_ &&
        velocity_z.size() >= particle_count_ && mass.size() >= particle_count_;
    const blitzar_status output_status = SynchronizeSimulationStatus(
        mpi_context_,
        output_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT,
        "get-state-preflight");
    if (output_status != BLITZAR_STATUS_OK) {
        return Remember(output_status);
    }
    const bool local_state_valid =
        particles_.Count() == local_particle_count_ &&
        local_particle_count_ <= particle_ids_.size() &&
        blitzar_core::IsValid(particles_.State());
    const blitzar_status state_status = SynchronizeSimulationStatus(
        mpi_context_,
        local_state_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INTERNAL_ERROR,
        "get-state-state");
    if (state_status != BLITZAR_STATUS_OK) {
        return Remember(state_status);
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
    const bool step_ready =
        particles_ready_ && integrator_kind_ == BLITZAR_INTEGRATOR_LEAPFROG_KDK &&
        std::isfinite(timestep_) && timestep_ > 0.0;
    const blitzar_status preflight_status = SynchronizeSimulationStatus(
        mpi_context_,
        step_ready ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT,
        "step-preflight");
    if (preflight_status != BLITZAR_STATUS_OK) {
        return Remember(preflight_status);
    }
    blitzar_status status = std::visit(
        [this](auto& solver) {
            if (mpi_context_.IsDistributed()) {
                const std::size_t rollback_particle_count = particles_.Count();
                const std::size_t rollback_acceleration_count =
                    accelerations_.Count();
                const std::size_t rollback_workspace_count = workspace_.Count();
                const bool rollback_state_valid =
                    rollback_particle_count == local_particle_count_ &&
                    rollback_particle_count <= particle_ids_.size() &&
                    rollback_particle_count == rollback_acceleration_count &&
                    rollback_particle_count == rollback_workspace_count &&
                    rollback_particle_count <= source_particle_count_ &&
                    source_particle_count_ <= arena_->Count();
                const blitzar_status state_status =
                    SynchronizeSimulationStatus(
                        mpi_context_,
                        rollback_state_valid ? BLITZAR_STATUS_OK
                                              : BLITZAR_STATUS_INTERNAL_ERROR,
                        "step-state");
                if (state_status != BLITZAR_STATUS_OK) {
                    return state_status;
                }
                DistributedStepTransaction transaction(
                    *arena_,
                    particles_,
                    accelerations_,
                    workspace_,
                    std::span<std::uint64_t>(particle_ids_),
                    local_particle_count_,
                    source_particle_count_,
                    exchange_buffer_,
                    rollback_arena_buffer_,
                    rollback_force_buffer_,
                    rollback_exchange_buffer_);
                blitzar_status prepare_status = transaction.Prepare();
                prepare_status = SynchronizeSimulationStatus(
                    mpi_context_, prepare_status, "step-prepare");
                if (prepare_status != BLITZAR_STATUS_OK) {
                    transaction.Abort();
                    return prepare_status;
                }
                transaction.Begin();
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
                auto rollback = [&dispatcher, &transaction]() noexcept {
                    dispatcher.Abort();
                    transaction.Abort();
                };
                auto migrate_after_drift =
                    [this, rollback_particle_count](
                        blitzar_particles::ParticleBuffer& current_particles,
                        blitzar_particles::AccelerationBuffer& current_accelerations,
                        blitzar_integration::LeapfrogWorkspace& current_workspace)
                    -> blitzar_integration::detail::DriftTransition {
                    const bool migration_state_valid =
                        current_particles.Count() == rollback_particle_count &&
                        current_accelerations.Count() ==
                            rollback_particle_count &&
                        current_workspace.Count() == rollback_particle_count &&
                        local_particle_count_ == rollback_particle_count &&
                        rollback_particle_count <= particle_ids_.size();
                    blitzar_status migration_status =
                        SynchronizeSimulationStatus(
                            mpi_context_,
                            migration_state_valid
                                ? BLITZAR_STATUS_OK
                                : BLITZAR_STATUS_INTERNAL_ERROR,
                            "migrate-preflight");
                    if (migration_status != BLITZAR_STATUS_OK) {
                        return {migration_status, false};
                    }
                    blitzar_parallel::PacketBuffer migrated;
                    migration_status = mpi_exchange_.Migrate(
                        current_particles.State(),
                        std::span<const std::uint64_t>(particle_ids_)
                            .first(local_particle_count_),
                        migrated);
                    if (migration_status != BLITZAR_STATUS_OK) {
                        return {migration_status, false};
                    }
                    migration_status = StoreLocalPackets(
                        migrated,
                        *arena_,
                        current_particles,
                        current_accelerations,
                        current_workspace,
                        std::span<std::uint64_t>(particle_ids_),
                        particle_count_,
                        local_particle_count_);
                    migration_status = SynchronizeSimulationStatus(
                        mpi_context_, migration_status, "migrate-commit");
                    if (migration_status != BLITZAR_STATUS_OK) {
                        return {migration_status, false};
                    }
                    source_particle_count_ = local_particle_count_;
                    return {BLITZAR_STATUS_OK, true};
                };
                const blitzar_status advance_status = integrator_.Advance(
                    particles_,
                    accelerations_,
                    workspace_,
                    dispatcher,
                    timestep_,
                    execution_settings_,
                    traversal_workspace_,
                    particles_.State(),
                    migrate_after_drift,
                    rollback);
                if (advance_status != BLITZAR_STATUS_OK) {
                    rollback();
                } else {
                    transaction.Complete();
                    transaction.Commit();
                }
                return advance_status;
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
    return Remember(status);
}

}  // namespace blitzar_sdk
