#ifndef BLITZAR_PARALLEL_MPI_EXCHANGE_GHOST_MPI_GHOST_STATE_HPP
#define BLITZAR_PARALLEL_MPI_EXCHANGE_GHOST_MPI_GHOST_STATE_HPP

#include "parallel/mpi/exchange/ghost/MpiGhostExchange.hpp"
#include "parallel/mpi/exchange/packets/PacketWire.hpp"
#include "parallel/mpi/native/MpiNative.hpp"

#include <cstddef>
#include <memory>
#include <vector>

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
    std::vector<std::byte> local_wire;
    std::vector<std::byte> receive_wire;
    std::vector<int> peer_counts;
    std::vector<std::size_t> peer_capacities;
    std::vector<std::size_t> wire_offsets;
    std::vector<std::size_t> receive_counts;
    std::vector<std::size_t> receive_byte_counts;
    std::vector<std::size_t> offsets;
    std::vector<ReceiveChunk> receive_chunks;
    std::unique_ptr<MpiNativeGhost> native;
};

} // namespace blitzar_parallel

#endif
