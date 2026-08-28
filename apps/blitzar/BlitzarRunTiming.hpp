#ifndef BLITZAR_APPS_BLITZAR_BLITZAR_RUN_TIMING_HPP
#define BLITZAR_APPS_BLITZAR_BLITZAR_RUN_TIMING_HPP

#include <cstdint>

namespace blitzar_cli {

struct BlitzarRunTiming final {
    std::uint64_t physics_elapsed_ns{};
    std::uint64_t output_elapsed_ns{};
    std::uint64_t output_checkpoint_count{};
};

} // namespace blitzar_cli

#endif
