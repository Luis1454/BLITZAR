#include "ScaleBlockSchedule.hpp"

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

std::uint64_t HashOwners(const std::array<std::uint8_t, MaxParticles>& owners) noexcept
{
    std::uint64_t hash = 0xcbf29ce484222325ULL;

    for (const std::uint8_t owner : owners) {
        hash ^= owner;
        hash *= 0x100000001b3ULL;
    }

    return hash;
}

} // namespace

ScaleBlockSchedule::ScaleBlockSchedule(const Workload& workload, bool use_time_bins) noexcept
    : workload_(workload), use_time_bins_(use_time_bins), owners_(workload.owners)
{
    result_.event_hash = 0xcbf29ce484222325ULL;
    result_.ownership_hash = HashOwners(owners_);
    result_.active_ordered = true;
}

bool ScaleBlockSchedule::ApplySynchronization() noexcept
{
    if (tick_ % SynchronizationInterval != 0) {
        return true;
    }

    ++result_.synchronization_count;

    if (!workload_.migration || tick_ != MigrationTick) {
        return true;
    }

    for (std::size_t index = 0; index < MaxParticles; ++index) {
        if (owners_[index] != workload_.migrated_owners[index]) {
            owners_[index] = workload_.migrated_owners[index];

            ++result_.migration_count;
        }
    }

    return true;
}

bool ScaleBlockSchedule::AppendActiveEvents() noexcept
{
    std::size_t previous_index = MaxParticles;
    bool has_previous = false;

    for (std::size_t index = 0; index < MaxParticles; ++index) {
        if (tick_ % Interval(index) != 0) {
            continue;
        }

        if (has_previous && index <= previous_index) {
            result_.active_ordered = false;
        }

        const std::uint64_t event = (static_cast<std::uint64_t>(tick_) << 32U) |
                                    (static_cast<std::uint64_t>(index) << 8U) | owners_[index];

        result_.event_hash = Mix(result_.event_hash ^ event);

        ++result_.event_count;

        previous_index = index;
        has_previous = true;
    }

    return result_.event_count <= MaxParticles * HorizonTicks;
}

std::uint32_t ScaleBlockSchedule::Interval(std::size_t index) const noexcept
{
    if (!use_time_bins_) {
        return 1;
    }

    return 1U << workload_.time_bins[index];
}

void ScaleBlockSchedule::UpdateOwnershipHash() noexcept
{
    result_.ownership_hash = HashOwners(owners_);
}

bool ScaleBlockSchedule::Advance() noexcept
{
    if (tick_ >= HorizonTicks || !ApplySynchronization() || !AppendActiveEvents()) {
        return false;
    }

    ++tick_;

    UpdateOwnershipHash();

    return true;
}

bool ScaleBlockSchedule::Capture(ScheduleSnapshot& snapshot) const noexcept
{
    snapshot.tick = tick_;
    snapshot.synchronization_count = result_.synchronization_count;
    snapshot.migration_count = result_.migration_count;
    snapshot.event_count = result_.event_count;
    snapshot.event_hash = result_.event_hash;
    snapshot.ownership_hash = result_.ownership_hash;
    snapshot.owners = owners_;

    return true;
}

bool ScaleBlockSchedule::Restore(const ScheduleSnapshot& snapshot) noexcept
{
    if (snapshot.tick > HorizonTicks || snapshot.event_count > MaxParticles * HorizonTicks ||
        snapshot.synchronization_count > HorizonTicks || snapshot.migration_count > MaxParticles) {
        return false;
    }

    tick_ = snapshot.tick;
    owners_ = snapshot.owners;
    result_.event_count = snapshot.event_count;
    result_.synchronization_count = snapshot.synchronization_count;
    result_.migration_count = snapshot.migration_count;
    result_.event_hash = snapshot.event_hash;
    result_.ownership_hash = snapshot.ownership_hash;
    result_.elapsed_ns = 0;
    result_.active_ordered = true;

    return true;
}

std::uint32_t ScaleBlockSchedule::Tick() const noexcept
{
    return tick_;
}

const ScheduleResult& ScaleBlockSchedule::Result() const noexcept
{
    return result_;
}

} // namespace blitzar_scaling_block
