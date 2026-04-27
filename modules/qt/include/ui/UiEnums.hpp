/*
 * @file modules/qt/include/ui/UiEnums.hpp
 * Centralized UI enums and converters for readability and consistency.
 */
#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_UIENUMS_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_UIENUMS_HPP_

#include <string>

namespace grav_qt {

enum class Solver { PairwiseCuda, OctreeGpu, OctreeCpu };
enum class Integrator { Euler, Rk4 };
enum class PerformanceProfile { Interactive, Balanced, Quality, Custom };

std::string to_string(Solver s);
std::string to_string(Integrator i);
std::string to_string(PerformanceProfile p);

Solver solver_from_string(const std::string& s);
Integrator integrator_from_string(const std::string& s);
PerformanceProfile performance_from_string(const std::string& s);

} // namespace grav_qt

#endif // GRAVITY_MODULES_QT_INCLUDE_UI_UIENUMS_HPP_
