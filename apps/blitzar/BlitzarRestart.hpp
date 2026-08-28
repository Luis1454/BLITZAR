#ifndef BLITZAR_APPS_BLITZAR_BLITZAR_RESTART_HPP
#define BLITZAR_APPS_BLITZAR_BLITZAR_RESTART_HPP

#include "simulation/config/SimConfigRun.hpp"
#include "simulation/initialization/SimConfigState.hpp"

#include <blitzar/blitzar.h>

namespace blitzar_cli {

[[nodiscard]] blitzar_status LoadRestartState(
    blitzar_sim::SimConfigRun& config, blitzar_sim::SimConfigState& destination) noexcept;

} // namespace blitzar_cli

#endif
