#ifndef BLITZAR_SDK_OVERLAP_CLOCK_HPP
#define BLITZAR_SDK_OVERLAP_CLOCK_HPP

#include <chrono>
#include <cstdint>

namespace blitzar_sdk {

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

} // namespace blitzar_sdk

#endif
