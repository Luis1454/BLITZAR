/**
 * @file UiEnums.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "ui/UiEnums.hpp"

namespace bltzr_qt {

/**
 * @brief Documents the solver to string operation contract.
 * @param s Input value used by this contract.
 * @return std::string value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 **/
std::string to_string(Solver s)
{
    switch (s) {
    case Solver::PairwiseCuda:
        return "pairwise_cuda";
    case Solver::OctreeGpu:
        return "octree_gpu";
    case Solver::OctreeCpu:
        return "octree_cpu";
    }
    return {};
}

/**
 * @brief Documents the integrator to string operation contract.
 * @param i Input value used by this contract.
 * @return std::string value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 **/
std::string to_string(Integrator i)
{
    switch (i) {
    case Integrator::Euler:
        return "euler";
    case Integrator::Rk4:
        return "rk4";
    }
    return {};
}

/**
 * @brief Documents the performance profile to string operation contract.
 * @param p Input value used by this contract.
 * @return std::string value produced by this contract.
 **/
std::string to_string(PerformanceProfile p)
{
    switch (p) {
    case PerformanceProfile::Interactive:
        return "interactive";
    case PerformanceProfile::Balanced:
        return "balanced";
    case PerformanceProfile::Quality:
        return "quality";
    case PerformanceProfile::Custom:
        return "custom";
    }
    return {};
}

/**
 * @brief Documents the solver from string operation contract.
 * @param s Input value used by this contract.
 * @return Solver value produced by this contract.
 **/
Solver solver_from_string(const std::string& s)
{
    if (s == "pairwise_cuda")
        return Solver::PairwiseCuda;
    if (s == "octree_gpu")
        return Solver::OctreeGpu;
    return Solver::OctreeCpu;
}

/**
 * @brief Documents the integrator from string operation contract.
 * @param s Input value used by this contract.
 * @return Integrator value produced by this contract.
 */
Integrator integrator_from_string(const std::string& s)
{
    if (s == "rk4")
        return Integrator::Rk4;
    return Integrator::Euler;
}

/**
 * @brief Documents the performance profile from string operation contract.
 * @param s Input value used by this contract.
 * @return PerformanceProfile value produced by this contract.
 **/
PerformanceProfile performance_from_string(const std::string& s)
{
    if (s == "balanced")
        return PerformanceProfile::Balanced;
    if (s == "quality")
        return PerformanceProfile::Quality;
    if (s == "custom")
        return PerformanceProfile::Custom;
    return PerformanceProfile::Interactive;
}

} // namespace bltzr_qt
