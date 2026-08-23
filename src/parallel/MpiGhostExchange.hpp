#ifndef BLITZAR_PARALLEL_MPI_GHOST_EXCHANGE_HPP
#define BLITZAR_PARALLEL_MPI_GHOST_EXCHANGE_HPP

#include <memory>

namespace blitzar_parallel {

class MpiGhostTransport;

class MpiGhostExchange final {
public:
    MpiGhostExchange() noexcept;
    ~MpiGhostExchange() noexcept;

    MpiGhostExchange(const MpiGhostExchange&) = delete;
    MpiGhostExchange& operator=(const MpiGhostExchange&) = delete;
    MpiGhostExchange(MpiGhostExchange&& other) noexcept;
    MpiGhostExchange& operator=(MpiGhostExchange&& other) noexcept;

private:
    friend class MpiGhostTransport;

    struct Impl;

    std::unique_ptr<Impl> impl_;
};

} // namespace blitzar_parallel

#endif
