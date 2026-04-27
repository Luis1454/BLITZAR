/*
 * @file modules/qt/include/ui/UiEnums.hpp
 * Centralized UI enums and converters for readability and consistency.
 */
#pragma once

#include <string>

namespace grav_qt {

enum class Solver { PairwiseCuda, OctreeGpu, OctreeCpu };
enum class Integrator { Euler, Rk4 };
enum class PerformanceProfile { Interactive, Balanced, Quality, Custom };

// Converters: enum -> string
static inline std::string to_string(Solver s)
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

static inline std::string to_string(Integrator i)
{
    switch (i) {
    case Integrator::Euler:
        return "euler";
    case Integrator::Rk4:
        return "rk4";
    }
    return {};
}

static inline std::string to_string(PerformanceProfile p)
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

// Converters: string -> enum (returns default on unknown)
static inline Solver solver_from_string(const std::string& s)
{
    if (s == "pairwise_cuda")
        return Solver::PairwiseCuda;
    if (s == "octree_gpu")
        return Solver::OctreeGpu;
    return Solver::OctreeCpu;
}

static inline Integrator integrator_from_string(const std::string& s)
{
    if (s == "rk4")
        return Integrator::Rk4;
    return Integrator::Euler;
}

static inline PerformanceProfile performance_from_string(const std::string& s)
{
    if (s == "balanced")
        return PerformanceProfile::Balanced;
    if (s == "quality")
        return PerformanceProfile::Quality;
    if (s == "custom")
        return PerformanceProfile::Custom;
    return PerformanceProfile::Interactive;
}

} // namespace grav_qt
