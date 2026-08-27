#ifndef BLITZAR_SIMULATION_CONFIG_SIM_CONFIG_DIAGNOSTICS_HPP
#define BLITZAR_SIMULATION_CONFIG_SIM_CONFIG_DIAGNOSTICS_HPP

#include <blitzar/blitzar.h>
#include <cstdint>

namespace blitzar_sim {

struct SimConfigDiagnostics final {
    bool enabled{};
    std::int64_t every_steps{1};
    bool energy{};
    bool momentum{};
    bool relative_error{};
};

} // namespace blitzar_sim

#endif
