#ifndef BLITZAR_TESTS_SCALING_SCALE_BLOCK_SCHEDULE_HPP
#define BLITZAR_TESTS_SCALING_SCALE_BLOCK_SCHEDULE_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace blitzar_scaling_block {

inline constexpr std::size_t MaxParticles = 32;
inline constexpr std::uint32_t HorizonTicks = 64;
inline constexpr std::uint32_t SynchronizationInterval = 8;
inline constexpr std::uint32_t MigrationTick = 32;
inline constexpr std::uint8_t MaxTimeBin = 3;

enum class WorkloadKind : std::uint8_t {
    Heterogeneous,
    Clustered,
    Migration,
};

struct Workload final {
    WorkloadKind kind{};
    std::string_view name{};
    std::array<std::uint8_t, MaxParticles> time_bins{};
    std::array<std::uint8_t, MaxParticles> owners{};
    std::array<std::uint8_t, MaxParticles> migrated_owners{};
    bool migration{false};
};

struct ScheduleSnapshot final {
    std::uint32_t tick{};
    std::uint32_t synchronization_count{};
    std::uint32_t migration_count{};
    std::uint64_t event_count{};
    std::uint64_t event_hash{};
    std::uint64_t ownership_hash{};
    std::array<std::uint8_t, MaxParticles> owners{};
};

struct ScheduleResult final {
    std::uint64_t event_count{};
    std::uint32_t synchronization_count{};
    std::uint32_t migration_count{};
    std::uint64_t event_hash{};
    std::uint64_t ownership_hash{};
    std::uint64_t elapsed_ns{};
    bool active_ordered{false};
};

struct QualificationResult final {
    std::string_view workload{};
    std::uint64_t fixed_event_count{};
    std::uint64_t block_event_count{};
    std::uint64_t fixed_elapsed_ns{};
    std::uint64_t block_elapsed_ns{};
    double modeled_speedup{};
    std::uint64_t fixed_event_hash{};
    std::uint64_t block_event_hash{};
    std::uint64_t restart_event_hash{};
    std::uint64_t rollback_event_hash{};
    std::uint64_t fixed_ownership_hash{};
    std::uint64_t block_ownership_hash{};
    std::uint64_t initial_input_hash{};
    std::uint64_t final_input_hash{};
    bool active_ordered{false};
    bool deterministic{false};
    bool ledger_conserved{false};
    bool migration{false};
    bool restart_compatible{false};
    bool rollback_transactional{false};
    bool state_unchanged{false};
    bool candidate_selected{false};
};

class ScaleBlockSchedule final {
public:
    ScaleBlockSchedule(const Workload& workload, bool use_time_bins) noexcept;

    [[nodiscard]] bool Advance() noexcept;
    [[nodiscard]] bool Capture(ScheduleSnapshot& snapshot) const noexcept;
    [[nodiscard]] bool Restore(const ScheduleSnapshot& snapshot) noexcept;
    [[nodiscard]] std::uint32_t Tick() const noexcept;
    [[nodiscard]] const ScheduleResult& Result() const noexcept;

private:
    [[nodiscard]] bool ApplySynchronization() noexcept;
    [[nodiscard]] bool AppendActiveEvents() noexcept;
    [[nodiscard]] std::uint32_t Interval(std::size_t index) const noexcept;
    void UpdateOwnershipHash() noexcept;

    const Workload& workload_;
    bool use_time_bins_;
    std::uint32_t tick_{0};
    std::array<std::uint8_t, MaxParticles> owners_{};
    ScheduleResult result_{};
};

[[nodiscard]] Workload MakeWorkload(WorkloadKind kind) noexcept;
[[nodiscard]] std::string_view WorkloadName(WorkloadKind kind) noexcept;
[[nodiscard]] std::uint64_t WorkloadHash(const Workload& workload) noexcept;
[[nodiscard]] bool RunQualification(const Workload& workload, QualificationResult& result) noexcept;

} // namespace blitzar_scaling_block

#endif
