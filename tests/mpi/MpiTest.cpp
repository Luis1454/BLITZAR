#include "MpiCases.hpp"
#include "fixtures/FixtureCheck.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string_view>

#if defined(BLITZAR_HAS_MPI)
#include <mpi.h>
#endif

#if defined(__linux__)
#include <sys/resource.h>
#endif

namespace {

struct ModeSelection final {
    bool known = false;
    bool single_rank = false;
    bool migration = false;
    bool barnes_hut = false;
    bool out_of_domain = false;
    bool large_count = false;
    bool overlap = false;
};

struct CaseResult final {
    std::string_view phase;
    std::string_view name;
    bool passed = false;
};

using CaseResults = std::array<CaseResult, 13>;

ModeSelection SelectMode(std::string_view mode) noexcept
{
    return {mode.empty() || mode == "single" || mode == "migration" ||
                mode == "barnes-hut-migration" || mode == "large-count" ||
                mode == "out-of-domain" || mode == "overlap" || mode == "internal",
        mode == "single", mode == "migration" || mode == "barnes-hut-migration",
        mode == "barnes-hut-migration", mode == "out-of-domain", mode == "large-count",
        mode == "overlap"};
}

bool IsValidWorld(
    const blitzar_parallel::MpiContext& context, const ModeSelection& selection) noexcept
{
    if (!context.IsUsable() || !selection.known) {
        return false;
    }

    if (selection.single_rank) {
        return context.Size() == 1;
    }

    return context.Size() == 2 || context.Size() == 4;
}

bool RunOutOfDomainCase(const ModeSelection& selection) noexcept
{
    return !selection.out_of_domain || blitzar_mpi_tests::RunOutOfDomainCase();
}

bool RunLargeCountCase(
    const blitzar_parallel::MpiContext& context, const ModeSelection& selection) noexcept
{
    return !selection.large_count || blitzar_mpi_tests::RunLargeCountValidationCase(context);
}

bool RunOverlapCase(blitzar_parallel::MpiContext& context, const ModeSelection& selection) noexcept
{
    return !selection.overlap || blitzar_mpi_tests::RunOverlapCase(context);
}

CaseResults RunCases(blitzar_parallel::MpiContext& context, const ModeSelection& selection) noexcept
{
    const blitzar_mpi_tests::StateArrays state = selection.migration
                                                     ? blitzar_mpi_tests::MigrationState()
                                                     : blitzar_mpi_tests::InitialState();

    const bool local_case = blitzar_mpi_tests::RunCase(state, 0.01, selection.migration ? 1 : 2,
        selection.barnes_hut ? BLITZAR_SOLVER_BARNES_HUT : BLITZAR_SOLVER_DIRECT);

    const bool allocation_case = blitzar_mpi_tests::RunAllocationCase();
    const bool migration_allocation_case = blitzar_mpi_tests::RunMigrationAllocationCase();
    const bool rollback_case = blitzar_mpi_tests::RunRollbackCase();
    const bool boundary_case = blitzar_mpi_tests::RunBoundaryOwnershipCase(context);
    const bool error_synchronization_case = blitzar_mpi_tests::RunErrorSynchronizationCase(context);
    const bool nested_context_case = blitzar_mpi_tests::RunNestedContextCase(context);
    const bool collective_validation_case = blitzar_mpi_tests::RunCollectiveValidationCase(context);
    const bool out_of_domain_case = RunOutOfDomainCase(selection);
    const bool large_count_case = RunLargeCountCase(context, selection);
    const bool overlap_case = RunOverlapCase(context, selection);

    const bool wire_codec_case = blitzar_mpi_tests::RunWireCodecCase();

    return {{{"P7", "world", IsValidWorld(context, selection)},
        {"P7", "local-distribution", local_case},
        {"P1", "steady-state-allocation", allocation_case},
        {"P8", "migration-allocation", migration_allocation_case},
        {"P8", "rollback", rollback_case}, {"P7", "domain-boundary", boundary_case},
        {"P7", "error-synchronization", error_synchronization_case},
        {"P7", "nested-context", nested_context_case},
        {"P7", "collective-validation", collective_validation_case},
        {"P8", "out-of-domain", out_of_domain_case}, {"P7", "large-count", large_count_case},
        {"P8", "overlap", overlap_case}, {"P7", "wire-codec", wire_codec_case}}};
}

bool AllCasesPassed(const CaseResults& results) noexcept
{
    for (const CaseResult& result : results) {
        if (!result.passed) {
            return false;
        }
    }

    return true;
}

void ReportCaseFailures(
    const blitzar_parallel::MpiContext& context, const CaseResults& results) noexcept
{
    for (const CaseResult& result : results) {
        if (result.passed) {
            continue;
        }

        std::fprintf(stderr,
            "MPI test failure rank=%d size=%d phase=%.*s case=%.*s "
            "expected=pass actual=fail\n",
            context.Rank(), context.Size(), static_cast<int>(result.phase.size()),
            result.phase.data(), static_cast<int>(result.name.size()), result.name.data());
    }
}

std::uint64_t PeakResidentBytes() noexcept
{
#if defined(__linux__)
    struct rusage usage{};

    if (getrusage(RUSAGE_SELF, &usage) != 0 || usage.ru_maxrss < 0) {
        return 0;
    }

    return static_cast<std::uint64_t>(usage.ru_maxrss) * 1024;
#else
    return 0;
#endif
}

void ReportPeakResidentMemory(
    const blitzar_parallel::MpiContext& context, std::size_t particle_count) noexcept
{
    const std::uint64_t bytes = PeakResidentBytes();

    if (bytes != 0) {
        std::fprintf(stderr,
            "BLITZAR MPI memory rank=%d particles=%zu ranks=%d peak_rss_bytes=%llu\n",
            context.Rank(), particle_count, context.Size(), static_cast<unsigned long long>(bytes));
    }
}

} // namespace

int RunTests(int argc, char** argv)
{
    blitzar_parallel::MpiContext context;
    const std::string_view mode = argc > 1 ? argv[1] : std::string_view{};
    const ModeSelection selection = SelectMode(mode);
    const CaseResults results = RunCases(context, selection);

    ReportCaseFailures(context, results);
    ReportPeakResidentMemory(context, blitzar_mpi_tests::ParticleCount);

    const int local_failure = AllCasesPassed(results) ? 0 : 1;
    int global_failure = 0;

    BLITZAR_CHECK(context.ReduceMax(local_failure, global_failure) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(global_failure == 0);

    return 0;
}

int main(int argc, char** argv)
{
#if defined(BLITZAR_HAS_MPI)
    const std::string_view mode = argc > 1 ? argv[1] : std::string_view{};
    const bool internal_owner = mode == "internal";
    int external_owner = 0;

    if (!internal_owner) {
        int initialized = 0;

        if (MPI_Initialized(&initialized) != MPI_SUCCESS) {
            return 1;
        }

        if (initialized == 0) {
            int provided = MPI_THREAD_SINGLE;

            if (MPI_Init_thread(nullptr, nullptr, MPI_THREAD_MULTIPLE, &provided) != MPI_SUCCESS ||
                provided < MPI_THREAD_MULTIPLE) {
                return 1;
            }

            external_owner = 1;
        }
    }
#endif

    const int result = RunTests(argc, argv);

#if defined(BLITZAR_HAS_MPI)

    if (external_owner != 0) {
        int finalized = 0;

        if (MPI_Finalized(&finalized) != MPI_SUCCESS || finalized != 0 ||
            MPI_Finalize() != MPI_SUCCESS) {
            return 1;
        }
    }
#endif

    return result;
}
