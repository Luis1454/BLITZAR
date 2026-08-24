#ifndef BLITZAR_PARALLEL_MPI_TRACE_HPP
#define BLITZAR_PARALLEL_MPI_TRACE_HPP

#include <blitzar/blitzar.h>
#include <cstddef>
#include <cstdint>

namespace blitzar_parallel {

enum class MpiOverlapMode : std::uint8_t {
    Overlapped,
    Serialized,
};

struct MpiOverlapTrace final {
    MpiOverlapMode mode{MpiOverlapMode::Overlapped};
    blitzar_status status{BLITZAR_STATUS_OK};
    std::size_t local_packets{};
    std::size_t ghost_packets{};
    std::size_t send_bytes{};
    std::size_t receive_bytes{};
    std::uint64_t begin_end_ns{};
    std::uint64_t local_start_ns{};
    std::uint64_t local_end_ns{};
    std::uint64_t complete_start_ns{};
    std::uint64_t complete_end_ns{};
    std::uint64_t remote_start_ns{};
    std::uint64_t remote_end_ns{};
    std::uint64_t total_ns{};

    void Reset(MpiOverlapMode selected_mode) noexcept
    {
        *this = MpiOverlapTrace{};
        mode = selected_mode;
    }

    [[nodiscard]] bool HasOverlap() const noexcept
    {
        return mode == MpiOverlapMode::Overlapped && local_end_ns > begin_end_ns &&
               complete_end_ns > local_start_ns;
    }
};

} // namespace blitzar_parallel

#endif
