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

#if defined(BLITZAR_HAS_MPI)
    using Request = MPI_Request;
    using Status = MPI_Status;
#else
    struct Request final {};
    struct Status final {};
#endif

    bool active{false};
    std::size_t send_capacity{0};
    std::size_t receive_capacity{0};
    MpiGhostExchange::TransferStats transfer{};
    std::vector<std::byte> local_wire;
    std::vector<std::byte> receive_wire;
    std::vector<int> peer_counts;
    std::vector<std::size_t> peer_capacities;
    std::vector<std::size_t> wire_offsets;
    std::vector<std::size_t> receive_counts;
    std::vector<std::size_t> offsets;
    std::vector<Request> receive_requests;
    std::vector<Request> send_requests;
    std::vector<Status> receive_statuses;
    std::vector<ReceiveChunk> receive_chunks;
};

} // namespace blitzar_parallel

#endif
