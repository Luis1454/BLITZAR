#ifndef BLITZAR_CORE_SOLVER_HPP
#define BLITZAR_CORE_SOLVER_HPP

#include <cstdint>

namespace blitzar_core {

enum class SolverKind : std::uint8_t { Direct = 0, BarnesHut = 1, Fmm = 2, Pm = 3, TreePm = 4 };

} // namespace blitzar_core

#endif
