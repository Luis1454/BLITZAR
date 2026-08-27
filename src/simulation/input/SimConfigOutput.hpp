#ifndef BLITZAR_SIMULATION_INPUT_SIM_CONFIG_OUTPUT_HPP
#define BLITZAR_SIMULATION_INPUT_SIM_CONFIG_OUTPUT_HPP

#include <blitzar/blitzar.h>
#include <cstdint>
#include <filesystem>

namespace blitzar_sim {

struct SimConfigOutput final {
    bool enabled{};
    std::filesystem::path directory;
    std::int64_t every_steps{};
    bool write_initial{};
    bool write_final{};
};

[[nodiscard]] blitzar_status ResolveOutputDirectory(
    SimConfigOutput& output, const std::filesystem::path& config_directory) noexcept;

} // namespace blitzar_sim

#endif
