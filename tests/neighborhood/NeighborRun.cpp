#include "neighborhood/NeighborRun.hpp"

#include "neighborhood/NeighborCandidate.hpp"
#include "neighborhood/NeighborCase.hpp"
#include "neighborhood/NeighborReference.hpp"

#include <algorithm>
#include <chrono>
namespace {

std::uint64_t Mix(std::uint64_t hash, std::uint64_t value) noexcept
{
    hash ^= value;
    hash *= 1099511628211ULL;

    return hash;
}

std::uint64_t Elapsed(
    std::chrono::steady_clock::time_point begin, std::chrono::steady_clock::time_point end) noexcept
{
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();

    return static_cast<std::uint64_t>(elapsed > 0 ? elapsed : 1);
}

blitzar_neighborhood::CandidateRun RunPass(const blitzar_neighborhood::NeighborWorkload& workload,
    blitzar_neighborhood::CandidateKind candidate)
{
    blitzar_neighborhood::CandidateRun run{0, 0, 0, 0, 0, 0, 1469598103934665603ULL,
        1469598103934665603ULL, 1469598103934665603ULL, true, true, false};

    blitzar_neighborhood::NeighborCandidate index(workload, candidate);

    for (std::size_t step = 0; step < workload.parameters.steps; ++step) {
        const blitzar_neighborhood::NeighborFrame frame =
            blitzar_neighborhood::MakeFrame(workload, step);

        const bool regular_rebuild =
            step == 0 || (blitzar_neighborhood::IsMoving(workload.scenario) &&
                             candidate != blitzar_neighborhood::CandidateKind::Verlet);

        const bool verlet_rebuild =
            candidate == blitzar_neighborhood::CandidateKind::Verlet && index.NeedsRebuild(frame);

        if (regular_rebuild || verlet_rebuild) {
            const auto begin = std::chrono::steady_clock::now();

            run.finite = index.Build(frame) && run.finite;

            const auto end = std::chrono::steady_clock::now();

            run.build_ns += Elapsed(begin, end);

            ++run.rebuild_count;
        }

        const auto query_begin = std::chrono::steady_clock::now();
        const blitzar_neighborhood::NeighborSet result = index.Query(frame);
        const auto query_end = std::chrono::steady_clock::now();

        run.query_ns += Elapsed(query_begin, query_end);

        const blitzar_neighborhood::NeighborSet reference =
            blitzar_neighborhood::BuildReference(frame, workload.parameters.radius);

        run.neighbor_count += result.Count();
        run.reference_count += reference.Count();
        run.candidate_hash = Mix(run.candidate_hash, result.Hash());
        run.reference_hash = Mix(run.reference_hash, reference.Hash());
        run.ordering_hash = Mix(run.ordering_hash, result.OrderingHash());
        run.finite = run.finite && result.IsValid(frame.x.size());
        run.correct = run.correct && result.IsValid(frame.x.size()) &&
                      blitzar_neighborhood::AreEqual(result, reference);

        run.memory_bytes = std::max(run.memory_bytes, index.MemoryBytes());
    }

    return run;
}

bool SameRun(const blitzar_neighborhood::CandidateRun& left,
    const blitzar_neighborhood::CandidateRun& right) noexcept
{
    return left.rebuild_count == right.rebuild_count &&
           left.neighbor_count == right.neighbor_count &&
           left.reference_count == right.reference_count &&
           left.memory_bytes == right.memory_bytes && left.candidate_hash == right.candidate_hash &&
           left.reference_hash == right.reference_hash &&
           left.ordering_hash == right.ordering_hash && left.finite == right.finite &&
           left.correct == right.correct;
}

} // namespace

namespace blitzar_neighborhood {

CandidateRun MeasureCandidate(const NeighborWorkload& workload, CandidateKind candidate)
{
    CandidateRun first = RunPass(workload, candidate);
    const CandidateRun second = RunPass(workload, candidate);

    first.finite = first.finite && second.finite;
    first.correct = first.correct && second.correct;
    first.reference_count = second.reference_count;
    first.repeatable = SameRun(first, second);

    return first;
}

} // namespace blitzar_neighborhood
