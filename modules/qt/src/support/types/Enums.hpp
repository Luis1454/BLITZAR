/*
 * @file modules/qt/src/support/types/Enums.hpp
 * @brief Centralized UI enums and converters for readability and consistency.
 */
#ifndef BLITZAR_MODULES_QT_SRC_SUPPORT_TYPES_ENUMS_HPP_
#define BLITZAR_MODULES_QT_SRC_SUPPORT_TYPES_ENUMS_HPP_

#include <string>

namespace bltzr_qt {

enum class Solver {
    PairwiseCuda,
    OctreeGpu,
    OctreeCpu
};
enum class Integrator {
    Euler,
    Rk4
};
enum class PerformanceProfile {
    Interactive,
    Balanced,
    Quality,
    Custom
};

std::string to_string(Solver s);
std::string to_string(Integrator i);
std::string to_string(PerformanceProfile p);

Solver solver_from_string(const std::string& s);
Integrator integrator_from_string(const std::string& s);
PerformanceProfile performance_from_string(const std::string& s);

} // namespace bltzr_qt

#endif // BLITZAR_MODULES_QT_SRC_SUPPORT_TYPES_ENUMS_HPP_
