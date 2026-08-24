#include "Check.hpp"
#include "core/Solver.hpp"
#include "core/Types.hpp"
#include "gpu/HipContext.hpp"
#include "integration/LeapfrogKdk.hpp"
#include "parallel/MpiTypes.hpp"
#include "sdk/Dispatch.hpp"
#include "sdk/State.hpp"
#include "sdk/Transaction.hpp"
#include "solvers/gpu/HipKernel.hpp"
#include "solvers/barnes_hut/BarnesHutSolver.hpp"
#include "solvers/direct/DirectSolver.hpp"

#include <array>
#include <atomic>
#include <blitzar/blitzar.h>
#include <cstdint>
#include <span>
#include <type_traits>
#include <vector>

static_assert(std::is_aggregate_v<blitzar_core::ParticleView>);
static_assert(std::is_aggregate_v<blitzar_core::ParticleStateView>);
static_assert(std::is_aggregate_v<blitzar_core::MutableParticleView>);
static_assert(std::is_aggregate_v<blitzar_core::ParticleOutputView>);
static_assert(std::is_aggregate_v<blitzar_core::ForceView>);
static_assert(std::is_aggregate_v<blitzar_core::ForceRange>);
static_assert(std::is_aggregate_v<blitzar_parallel::AllToAllPacketRequest>);
static_assert(std::is_aggregate_v<blitzar_gpu::BarnesHutComputeRequest>);
static_assert(std::is_aggregate_v<blitzar_gpu_detail::DirectLaunchRequest>);
static_assert(std::is_aggregate_v<blitzar_gpu_detail::BarnesHutLaunchRequest>);

using DirectSolver = blitzar_direct::DirectSolver;
using SolverScratch = blitzar_barnes_hut::ThreadStackPool;

static_assert(std::is_aggregate_v<
              blitzar_integration_kdk::SolverComputeRequest<DirectSolver, SolverScratch>>);
static_assert(std::is_aggregate_v<
              blitzar_integration_kdk::AdvanceState<DirectSolver, SolverScratch>>);
static_assert(std::is_aggregate_v<blitzar_integration_kdk::AdvanceHooks<
              blitzar_integration_kdk::NoopDriftHook, blitzar_integration_kdk::NoopRollbackHook>>);
static_assert(std::is_aggregate_v<blitzar_sdk::PacketStoreRequest>);
static_assert(std::is_aggregate_v<blitzar_sdk::ArenaCaptureRequest>);
static_assert(std::is_aggregate_v<blitzar_sdk::ArenaRestoreRequest>);
static_assert(std::is_aggregate_v<blitzar_sdk::TransactionState>);

int main()
{
    std::array<blitzar_core::Scalar, 2> position_x{0.0, 1.0};
    std::array<blitzar_core::Scalar, 2> position_y{};
    std::array<blitzar_core::Scalar, 2> position_z{};
    std::array<blitzar_core::Scalar, 2> velocity_x{};
    std::array<blitzar_core::Scalar, 2> velocity_y{};
    std::array<blitzar_core::Scalar, 2> velocity_z{};
    std::array<blitzar_core::Scalar, 2> mass{1.0, 1.0};
    std::array<blitzar_core::Scalar, 2> force_x{};
    std::array<blitzar_core::Scalar, 2> force_y{};
    std::array<blitzar_core::Scalar, 2> force_z{};

    const blitzar_core::ParticleView particles{2, position_x, position_y, position_z};
    const blitzar_core::ParticleStateView state{2, position_x, position_y, position_z,
        velocity_x, velocity_y, velocity_z, mass};

    const blitzar_core::MutableParticleView mutable_state{2, position_x, position_y, position_z,
        velocity_x, velocity_y, velocity_z};

    const blitzar_core::ParticleOutputView output{2, position_x, position_y, position_z,
        velocity_x, velocity_y, velocity_z, mass};

    const blitzar_core::ForceView force{2, force_x, force_y, force_z};

    BLITZAR_CHECK(blitzar_core::IsValid(particles));
    BLITZAR_CHECK(blitzar_core::IsValid(state));
    BLITZAR_CHECK(blitzar_core::IsValid(mutable_state));
    BLITZAR_CHECK(blitzar_core::IsValid(output));
    BLITZAR_CHECK(blitzar_core::IsValid(force));
    BLITZAR_CHECK(state.SourceCount() == 2);

    const blitzar_core::ForceRange full_range{0, 2, false};
    const blitzar_core::ForceRange remote_range{1, 2, true};
    const blitzar_core::ForceRange invalid_range{2, 1, false};

    BLITZAR_CHECK(full_range.IsValid(2));
    BLITZAR_CHECK(remote_range.IsValid(2));
    BLITZAR_CHECK(!invalid_range.IsValid(2));

    blitzar_core::ExecutionSettings execution{};
    blitzar_physics::GravityParameters gravity{};
    blitzar_barnes_hut::BarnesHutSettings barnes_hut{};
    const blitzar_gpu::BarnesHutComputeRequest gpu_request{
        state, force, execution, gravity, barnes_hut};

    BLITZAR_CHECK(&gpu_request.execution == &execution);
    BLITZAR_CHECK(gpu_request.particles.count == state.count);

    const blitzar_gpu_detail::DeviceParticleAddresses addresses{};
    const blitzar_gpu_detail::KernelPhysics physics{};
    const blitzar_gpu_detail::KernelRuntime runtime{};
    const blitzar_gpu_detail::DirectLaunchRequest direct_launch{
        addresses, 2, full_range, physics, runtime};

    const blitzar_gpu_detail::BarnesHutLaunchRequest tree_launch{
        addresses, 2, 2, 0.5, {}, physics, 32, runtime};

    BLITZAR_CHECK(direct_launch.range.source_end == 2);
    BLITZAR_CHECK(tree_launch.tree.cell_count == 0);
    BLITZAR_CHECK(tree_launch.max_depth == 32);

    std::array<blitzar_parallel::ParticlePacket, 2> packets{};
    std::array<int, 1> counts{2};
    std::array<int, 1> displacements{};
    const blitzar_parallel::AllToAllPacketRequest packet_request{
        packets, counts, displacements, packets, counts, displacements};

    BLITZAR_CHECK(packet_request.send_packets.size() == 2);
    BLITZAR_CHECK(packet_request.receive_counts[0] == 2);

    blitzar_particles::ParticleArena arena(0);
    blitzar_particles::ParticleBuffer particle_buffer(arena);
    blitzar_particles::AccelerationBuffer acceleration_buffer(arena);
    blitzar_integration::KdkCheckpoint checkpoint(arena);
    blitzar_parallel::PacketBuffer exchange;
    std::array<std::uint64_t, 0> ids{};
    std::size_t local_count = 0;
    blitzar_sdk::ParticleInputStage stage{};

    const blitzar_sdk::PacketStoreRequest store_request{exchange, arena, particle_buffer,
        acceleration_buffer, checkpoint, ids, 0, local_count};

    const blitzar_sdk::ArenaCaptureRequest capture_request{arena, 0, ids, exchange};
    const blitzar_sdk::ArenaRestoreRequest restore_request{exchange, arena, particle_buffer, ids, 0};
    std::vector<std::uint64_t> transaction_ids;

    const blitzar_sdk::TransactionState transaction_state{arena, particle_buffer,
        acceleration_buffer, checkpoint, transaction_ids,
        local_count, exchange, exchange, exchange, exchange};

    BLITZAR_CHECK(store_request.particle_count == 0);
    BLITZAR_CHECK(capture_request.local_count == 0);
    BLITZAR_CHECK(restore_request.local_count == 0);
    BLITZAR_CHECK(&transaction_state.exchange == &exchange);

    const blitzar_core::ParticleStateView empty_stage = stage.State();

    BLITZAR_CHECK(blitzar_core::IsValid(empty_stage));

    DirectSolver direct(gravity, 0);
    SolverScratch solver_scratch(0, 0);
    blitzar_integration_kdk::SolverComputeRequest compute_request{
        direct, state, force, execution, solver_scratch};

    blitzar_integration_kdk::AdvanceState<DirectSolver, SolverScratch> advance_state{
        particle_buffer, acceleration_buffer, checkpoint, direct, 1.0, execution, solver_scratch,
        particle_buffer.State()};

    blitzar_integration_kdk::NoopDriftHook drift_hook;
    blitzar_integration_kdk::NoopRollbackHook rollback_hook;
    blitzar_integration_kdk::AdvanceHooks hooks{drift_hook, rollback_hook};
    blitzar_integration_kdk::AdvanceRequest advance_request{advance_state, hooks};

    BLITZAR_CHECK(&compute_request.solver == &direct);
    BLITZAR_CHECK(&advance_request.state == &advance_state);
    BLITZAR_CHECK(&advance_request.hooks == &hooks);

    return 0;
}
