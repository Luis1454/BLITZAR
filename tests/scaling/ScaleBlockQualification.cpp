#include "ScaleBlockSchedule.hpp"

#include <chrono>
#include <cstddef>

namespace blitzar_scaling_block {

namespace {

constexpr std::uint64_t Mix(std::uint64_t value) noexcept
{
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;

    return value ^ (value >> 31U);
}

bool ValidWorkload(const Workload& workload) noexcept
{
    for (std::size_t index = 0; index < MaxParticles; ++index) {
        if (workload.time_bins[index] > MaxTimeBin || workload.owners[index] > 1U ||
            workload.migrated_owners[index] > 1U) {
            return false;
        }
    }

    return workload.migration == (workload.kind == WorkloadKind::Migration);
}

bool RunUntil(ScaleBlockSchedule& schedule, std::uint32_t target_tick) noexcept
{
    while (schedule.Tick() < target_tick) {
        if (!schedule.Advance()) {
            return false;
        }
    }

    return true;
}

bool EqualSchedule(const ScheduleResult& left, const ScheduleResult& right) noexcept
{
    return left.event_count == right.event_count &&
           left.synchronization_count == right.synchronization_count &&
           left.migration_count == right.migration_count && left.event_hash == right.event_hash &&
           left.ownership_hash == right.ownership_hash &&
           left.active_ordered == right.active_ordered;
}

std::uint64_t ExpectedOwnershipHash(const Workload& workload) noexcept
{
    std::uint64_t hash = 0xcbf29ce484222325ULL;

    for (std::size_t index = 0; index < MaxParticles; ++index) {
        const std::uint8_t owner =
            workload.migration ? workload.migrated_owners[index] : workload.owners[index];

        hash ^= owner;
        hash *= 0x100000001b3ULL;
    }

    return hash;
}

bool ValidateMigration(
    const Workload& workload, const ScheduleResult& fixed, const ScheduleResult& block) noexcept
{
    std::uint32_t expected_migrations = 0;

    for (std::size_t index = 0; index < MaxParticles; ++index) {
        expected_migrations += workload.owners[index] != workload.migrated_owners[index];
    }

    const std::uint32_t expected_sync = HorizonTicks / SynchronizationInterval;

    return fixed.synchronization_count == expected_sync &&
           block.synchronization_count == expected_sync &&
           fixed.migration_count == expected_migrations &&
           block.migration_count == expected_migrations &&
           fixed.ownership_hash == ExpectedOwnershipHash(workload) &&
           block.ownership_hash == ExpectedOwnershipHash(workload) &&
           MigrationTick % SynchronizationInterval == 0;
}

bool RunSchedule(const Workload& workload, bool use_time_bins, ScheduleResult& result) noexcept
{
    ScaleBlockSchedule schedule(workload, use_time_bins);
    const auto start = std::chrono::steady_clock::now();

    while (schedule.Advance()) {
    }

    const auto end = std::chrono::steady_clock::now();

    result = schedule.Result();
    result.elapsed_ns = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());

    if (result.elapsed_ns == 0) {
        result.elapsed_ns = 1;
    }

    return schedule.Tick() == HorizonTicks;
}

bool RunReplay(const Workload& workload, std::uint64_t& replay_hash) noexcept
{
    ScaleBlockSchedule uninterrupted(workload, true);
    ScaleBlockSchedule resumed(workload, true);
    ScheduleSnapshot snapshot{};

    while (uninterrupted.Advance()) {
    }

    if (!RunUntil(resumed, HorizonTicks / 2U) || !resumed.Capture(snapshot) ||
        !resumed.Restore(snapshot) || !RunUntil(resumed, HorizonTicks)) {
        return false;
    }

    replay_hash = resumed.Result().event_hash;

    return EqualSchedule(uninterrupted.Result(), resumed.Result());
}

bool RunRollback(const Workload& workload, std::uint64_t& rollback_hash) noexcept
{
    ScaleBlockSchedule uninterrupted(workload, true);
    ScaleBlockSchedule retried(workload, true);
    ScheduleSnapshot snapshot{};

    while (uninterrupted.Advance()) {
    }

    if (!RunUntil(retried, MigrationTick - 1U) || !retried.Capture(snapshot) ||
        !retried.Advance() || !retried.Advance() || !retried.Restore(snapshot) ||
        !RunUntil(retried, HorizonTicks)) {
        return false;
    }

    rollback_hash = retried.Result().event_hash;

    return EqualSchedule(uninterrupted.Result(), retried.Result());
}

} // namespace

std::string_view WorkloadName(WorkloadKind kind) noexcept
{
    switch (kind) {
    case WorkloadKind::Heterogeneous:

        return "heterogeneous-v1";

    case WorkloadKind::Clustered:

        return "clustered-v1";

    case WorkloadKind::Migration:

        return "migration-v1";
    }

    return "unknown";
}

Workload MakeWorkload(WorkloadKind kind) noexcept
{
    Workload workload{};

    workload.kind = kind;
    workload.name = WorkloadName(kind);

    for (std::size_t index = 0; index < MaxParticles; ++index) {
        workload.owners[index] = static_cast<std::uint8_t>(index % 2U);
        workload.migrated_owners[index] = workload.owners[index];
    }

    if (kind == WorkloadKind::Clustered) {
        for (std::size_t index = MaxParticles / 2U; index < MaxParticles; ++index) {
            workload.time_bins[index] = MaxTimeBin;
        }
    }
    else {
        for (std::size_t index = 0; index < MaxParticles; ++index) {
            workload.time_bins[index] = static_cast<std::uint8_t>(index % (MaxTimeBin + 1U));
        }
    }

    workload.migration = kind == WorkloadKind::Migration;

    if (workload.migration) {
        for (std::size_t index = 0; index < MaxParticles; ++index) {
            workload.migrated_owners[index] =
                static_cast<std::uint8_t>(1U - workload.owners[index]);
        }
    }

    return workload;
}

std::uint64_t WorkloadHash(const Workload& workload) noexcept
{
    std::uint64_t hash = 0xcbf29ce484222325ULL;

    hash = Mix(hash ^ static_cast<std::uint8_t>(workload.kind));
    hash = Mix(hash ^ static_cast<std::uint8_t>(workload.migration));

    for (std::size_t index = 0; index < MaxParticles; ++index) {
        hash = Mix(hash ^ workload.time_bins[index]);
        hash = Mix(hash ^ workload.owners[index]);
        hash = Mix(hash ^ workload.migrated_owners[index]);
    }

    return hash;
}

bool RunQualification(const Workload& workload, QualificationResult& result) noexcept
{
    result = {};
    result.workload = workload.name;

    if (!ValidWorkload(workload)) {
        return false;
    }

    result.initial_input_hash = WorkloadHash(workload);

    ScheduleResult fixed_first{};
    ScheduleResult fixed_second{};
    ScheduleResult block_first{};
    ScheduleResult block_second{};

    if (!RunSchedule(workload, false, fixed_first) || !RunSchedule(workload, false, fixed_second) ||
        !RunSchedule(workload, true, block_first) || !RunSchedule(workload, true, block_second)) {
        return false;
    }

    result.fixed_event_count = fixed_first.event_count;
    result.block_event_count = block_first.event_count;
    result.final_input_hash = WorkloadHash(workload);
    result.fixed_elapsed_ns = fixed_first.elapsed_ns;
    result.block_elapsed_ns = block_first.elapsed_ns;
    result.modeled_speedup =
        static_cast<double>(fixed_first.event_count) / static_cast<double>(block_first.event_count);

    result.fixed_event_hash = fixed_first.event_hash;
    result.block_event_hash = block_first.event_hash;
    result.fixed_ownership_hash = fixed_first.ownership_hash;
    result.block_ownership_hash = block_first.ownership_hash;
    result.active_ordered = block_first.active_ordered;
    result.deterministic =
        EqualSchedule(fixed_first, fixed_second) && EqualSchedule(block_first, block_second);

    result.ledger_conserved =
        fixed_first.event_count == MaxParticles * HorizonTicks && block_first.event_count > 0;

    result.migration = ValidateMigration(workload, fixed_first, block_first);
    result.state_unchanged = result.initial_input_hash == WorkloadHash(workload);
    result.restart_compatible = RunReplay(workload, result.restart_event_hash);
    result.rollback_transactional = RunRollback(workload, result.rollback_event_hash);
    result.candidate_selected = false;

    return result.fixed_elapsed_ns > 0 && result.block_elapsed_ns > 0 &&
           result.block_event_count < result.fixed_event_count && result.modeled_speedup > 1.0 &&
           result.fixed_ownership_hash == result.block_ownership_hash && result.deterministic &&
           result.active_ordered && result.ledger_conserved && result.migration &&
           result.state_unchanged && result.restart_compatible && result.rollback_transactional;
}

} // namespace blitzar_scaling_block
