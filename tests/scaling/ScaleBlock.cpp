#include "ScaleBlockSchedule.hpp"
#include "fixtures/FixtureCheck.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string_view>

namespace {

void PrintResult(const blitzar_scaling_block::QualificationResult& result) noexcept
{
    std::fprintf(stdout,
        "BLITZAR BLOCK schema=1 seed=424242 workload=%.*s particles=%zu horizon_ticks=%u "
        "sync_interval=%u migration_tick=%u fixed_events=%llu block_events=%llu "
        "fixed_elapsed_ns=%llu block_elapsed_ns=%llu modeled_speedup=%.17g "
        "fixed_event_hash=%llu block_event_hash=%llu restart_event_hash=%llu "
        "rollback_event_hash=%llu fixed_ownership_hash=%llu block_ownership_hash=%llu "
        "initial_input_hash=%llu final_input_hash=%llu reference=fixed-kdk-v1 "
        "candidate=block-kdk-schedule-v1 speedup_scope=schedule-work-proxy "
        "decision=not-selected active_ordered=%d deterministic=%d ledger_conserved=%d "
        "migration=%d restart_compatible=%d rollback_transactional=%d state_unchanged=%d "
        "candidate_selected=%d\n",
        static_cast<int>(result.workload.size()), result.workload.data(),
        blitzar_scaling_block::MaxParticles, blitzar_scaling_block::HorizonTicks,
        blitzar_scaling_block::SynchronizationInterval, blitzar_scaling_block::MigrationTick,
        static_cast<unsigned long long>(result.fixed_event_count),
        static_cast<unsigned long long>(result.block_event_count),
        static_cast<unsigned long long>(result.fixed_elapsed_ns),
        static_cast<unsigned long long>(result.block_elapsed_ns), result.modeled_speedup,
        static_cast<unsigned long long>(result.fixed_event_hash),
        static_cast<unsigned long long>(result.block_event_hash),
        static_cast<unsigned long long>(result.restart_event_hash),
        static_cast<unsigned long long>(result.rollback_event_hash),
        static_cast<unsigned long long>(result.fixed_ownership_hash),
        static_cast<unsigned long long>(result.block_ownership_hash),
        static_cast<unsigned long long>(result.initial_input_hash),
        static_cast<unsigned long long>(result.final_input_hash), result.active_ordered ? 1 : 0,
        result.deterministic ? 1 : 0, result.ledger_conserved ? 1 : 0, result.migration ? 1 : 0,
        result.restart_compatible ? 1 : 0, result.rollback_transactional ? 1 : 0,
        result.state_unchanged ? 1 : 0, result.candidate_selected ? 1 : 0);
}

bool ValidateResult(const blitzar_scaling_block::QualificationResult& result) noexcept
{
    return result.fixed_event_count > result.block_event_count && result.modeled_speedup > 1.0 &&
           result.fixed_event_hash != result.block_event_hash &&
           result.restart_event_hash == result.block_event_hash &&
           result.rollback_event_hash == result.block_event_hash &&
           result.fixed_ownership_hash == result.block_ownership_hash &&
           result.initial_input_hash == result.final_input_hash && result.active_ordered &&
           result.deterministic && result.ledger_conserved && result.migration &&
           result.restart_compatible && result.rollback_transactional && result.state_unchanged &&
           !result.candidate_selected;
}

} // namespace

int main()
{
    constexpr std::array<blitzar_scaling_block::WorkloadKind, 3> kinds{
        blitzar_scaling_block::WorkloadKind::Heterogeneous,
        blitzar_scaling_block::WorkloadKind::Clustered,
        blitzar_scaling_block::WorkloadKind::Migration};

    for (const auto kind : kinds) {
        const blitzar_scaling_block::Workload workload = blitzar_scaling_block::MakeWorkload(kind);
        blitzar_scaling_block::QualificationResult result{};

        BLITZAR_CHECK(blitzar_scaling_block::RunQualification(workload, result));
        BLITZAR_CHECK(ValidateResult(result));

        PrintResult(result);
    }

    return 0;
}
