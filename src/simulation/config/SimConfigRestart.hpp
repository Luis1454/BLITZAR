#ifndef BLITZAR_SIMULATION_CONFIG_SIM_CONFIG_RESTART_HPP
#define BLITZAR_SIMULATION_CONFIG_SIM_CONFIG_RESTART_HPP

#include <blitzar/blitzar.h>
#include <cstdint>
#include <filesystem>

namespace blitzar_sim {

struct SimConfigRestart final {
    bool enabled{};
    std::filesystem::path directory;
    std::uint64_t step{};
    double time{};
};

[[nodiscard]] blitzar_status ResolveRestartDirectory(
    SimConfigRestart& restart, const std::filesystem::path& config_directory) noexcept;

} // namespace blitzar_sim

#endif
