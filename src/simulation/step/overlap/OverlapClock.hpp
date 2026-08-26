#ifndef BLITZAR_SIMULATION_STEP_OVERLAP_OVERLAP_CLOCK_HPP
#define BLITZAR_SIMULATION_STEP_OVERLAP_OVERLAP_CLOCK_HPP

#include <chrono>
#include <cstdint>

namespace blitzar_sim {

class OverlapClock final {
public:
    using Clock = std::chrono::steady_clock;
    using Time = Clock::time_point;

    [[nodiscard]] static std::uint64_t Elapsed(Time start, Time end) noexcept
    {
        return static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
    }
};

} // namespace blitzar_sim

#endif
