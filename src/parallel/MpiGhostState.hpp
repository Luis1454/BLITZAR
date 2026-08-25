#ifndef BLITZAR_PARALLEL_MPI_GHOST_STATE_HPP
#define BLITZAR_PARALLEL_MPI_GHOST_STATE_HPP

#include "parallel/MpiGhostExchange.hpp"
#include "parallel/MpiTypes.hpp"

#include <cstddef>
#include <vector>

#if defined(BLITZAR_HAS_MPI)
#include <mpi.h>
#endif

namespace blitzar_parallel {

struct MpiGhostExchange::Impl final {
    struct ReceiveChunk final {
        std::size_t peer_index{};
        std::size_t packet_offset{};
    };

    bool active{false};
    std::size_t send_capacity{0};
    std::size_t receive_capacity{0};
    MpiGhostExchange::TransferStats transfer{};
#if defined(BLITZAR_HAS_MPI)
    std::vector<std::byte> local_wire;
    std::vector<std::byte> receive_wire;
    std::vector<int> peer_counts;
    std::vector<std::size_t> peer_capacities;
    std::vector<std::size_t> wire_offsets;
    std::vector<std::size_t> receive_counts;
    std::vector<std::size_t> offsets;
    std::vector<MPI_Request> receive_requests;
    std::vector<MPI_Request> send_requests;
    std::vector<MPI_Status> receive_statuses;
    std::vector<ReceiveChunk> receive_chunks;
#endif
};

} // namespace blitzar_parallel

#endif
