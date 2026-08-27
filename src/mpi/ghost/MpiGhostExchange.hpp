#ifndef BLITZAR_MPI_GHOST_MPI_GHOST_EXCHANGE_HPP
#define BLITZAR_MPI_GHOST_MPI_GHOST_EXCHANGE_HPP

#include <cstddef>
#include <memory>

namespace blitzar_parallel {

class MpiGhostTransport;

class MpiGhostExchange final {
public:
    struct TransferStats final {
        std::size_t send_bytes{};
        std::size_t receive_bytes{};
    };

    MpiGhostExchange() noexcept;
    ~MpiGhostExchange() noexcept;

    MpiGhostExchange(const MpiGhostExchange&) = delete;
    MpiGhostExchange& operator=(const MpiGhostExchange&) = delete;
    MpiGhostExchange(MpiGhostExchange&& other) noexcept;
    MpiGhostExchange& operator=(MpiGhostExchange&& other) noexcept;

    [[nodiscard]] TransferStats Transfer() const noexcept;

private:
    friend class MpiGhostTransport;

    struct Impl;

    std::unique_ptr<Impl> impl_;
};

} // namespace blitzar_parallel

#endif
