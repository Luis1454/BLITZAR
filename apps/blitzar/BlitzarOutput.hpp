#ifndef BLITZAR_APPS_BLITZAR_BLITZAR_OUTPUT_HPP
#define BLITZAR_APPS_BLITZAR_BLITZAR_OUTPUT_HPP

#include "core/CoreTypes.hpp"
#include "io/metadata/MetadataRun.hpp"
#include "simulation/config/SimConfigRun.hpp"

#include <blitzar/blitzar.h>
#include <cstdint>
#include <optional>
#include <vector>

namespace blitzar_cli {

class BlitzarOutput final {
public:
    explicit BlitzarOutput(const blitzar_sim::SimConfigRun& config) noexcept;

    [[nodiscard]] blitzar_status Prepare() noexcept;
    [[nodiscard]] bool ShouldWriteInitial() const noexcept;
    [[nodiscard]] bool ShouldWriteStep(std::uint64_t step) const noexcept;
    [[nodiscard]] blitzar_status Publish(
        std::uint64_t step, blitzar_core::ParticleOutputView state) noexcept;

private:
    const blitzar_sim::SimConfigRun& config_;
    std::optional<blitzar_io::MetadataRun> run_;
    std::vector<std::uint64_t> ids_;
    bool prepared_{};
};

} // namespace blitzar_cli

#endif
