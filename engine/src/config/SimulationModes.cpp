#include "config/SimulationModes.hpp"

#include <algorithm>
#include <cctype>

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
namespace grav_modes {

const std::string_view kSolverPairwiseCuda = "pairwise_cuda";
const std::string_view kSolverOctreeGpu = "octree_gpu";
const std::string_view kSolverOctreeCpu = "octree_cpu";
const std::string_view kIntegratorEuler = "euler";
const std::string_view kIntegratorRk4 = "rk4";
const std::string_view kIntegratorLeapfrog = "leapfrog";
const std::string_view kOctreeCriterionCom = "com";
const std::string_view kOctreeCriterionBounds = "bounds";

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
    if (normalized == "leapfrog" || normalized == "kdk" || normalized == "verlet") {
        outCanonical.assign(kIntegratorLeapfrog);
        return true;
    }
    if (normalized == "rk4" || normalized == "runge_kutta4" || normalized == "runge-kutta4") {
        outCanonical.assign(kIntegratorRk4);
        return true;
    }
    return false;
}

bool normalizeOctreeOpeningCriterion(std::string_view value, std::string &outCanonical)
{
    const std::string normalized = toLowerTrimmed(value);
    if (normalized == "com" || normalized == "center" || normalized == "center_distance" || normalized == "center-distance") {
        outCanonical.assign(kOctreeCriterionCom);
        return true;
    }
    if (normalized == "bounds" || normalized == "box" || normalized == "bounds_distance" || normalized == "bounds-distance") {
        outCanonical.assign(kOctreeCriterionBounds);
        return true;
    }
    return false;
}

bool isSupportedSolverIntegratorPair(std::string_view solver, std::string_view integrator)
{
    return !(solver == kSolverOctreeGpu && integrator == kIntegratorRk4);
}

} // namespace grav_modes

