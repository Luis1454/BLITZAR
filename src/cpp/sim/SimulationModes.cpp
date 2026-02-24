#include "sim/SimulationModes.hpp"

#include <algorithm>
#include <cctype>

namespace {

std::string toLowerTrimmed(std::string_view value)
{
    std::size_t begin = 0u;
    std::size_t end = value.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(value[begin])) != 0) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1u])) != 0) {
        --end;
    }

    std::string normalized(value.substr(begin, end - begin));
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return normalized;
}

} // namespace

namespace sim::modes {

bool normalizeSolver(std::string_view value, std::string &outCanonical)
{
    const std::string normalized = toLowerTrimmed(value);
    if (normalized == "pairwise_cuda" || normalized == "pairwise" || normalized == "pairwise-cuda") {
        outCanonical.assign(kSolverPairwiseCuda);
        return true;
    }
    if (normalized == "octree_gpu" || normalized == "octree-gpu") {
        outCanonical.assign(kSolverOctreeGpu);
        return true;
    }
    if (normalized == "octree_cpu" || normalized == "octree-cpu" || normalized == "octree") {
        outCanonical.assign(kSolverOctreeCpu);
        return true;
    }
    return false;
}

bool normalizeIntegrator(std::string_view value, std::string &outCanonical)
{
    const std::string normalized = toLowerTrimmed(value);
    if (normalized == "euler") {
        outCanonical.assign(kIntegratorEuler);
        return true;
    }
    if (normalized == "rk4" || normalized == "runge_kutta4" || normalized == "runge-kutta4") {
        outCanonical.assign(kIntegratorRk4);
        return true;
    }
    return false;
}

} // namespace sim::modes
