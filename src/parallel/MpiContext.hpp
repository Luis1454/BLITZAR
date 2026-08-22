#ifndef BLITZAR_PARALLEL_MPI_CONTEXT_HPP
#define BLITZAR_PARALLEL_MPI_CONTEXT_HPP

#include <blitzar/blitzar.h>

#include <cstddef>
#include <cstdint>

#if defined(BLITZAR_HAS_MPI)
#include <mpi.h>
#endif

namespace blitzar_parallel {

#if defined(BLITZAR_HAS_MPI)
using MpiCommunicator = MPI_Comm;
#else
using MpiCommunicator = std::uintptr_t;
#endif

class MpiContext final {
public:
    MpiContext() noexcept;
    ~MpiContext() noexcept;

    MpiContext(const MpiContext&) = delete;
    MpiContext& operator=(const MpiContext&) = delete;
    MpiContext(MpiContext&&) = delete;
    MpiContext& operator=(MpiContext&&) = delete;

    [[nodiscard]] bool IsUsable() const noexcept;
    [[nodiscard]] bool IsDistributed() const noexcept;
    [[nodiscard]] int Rank() const noexcept;
    [[nodiscard]] int Size() const noexcept;
    [[nodiscard]] MpiCommunicator Communicator() const noexcept;
    [[nodiscard]] blitzar_status Status() const noexcept;

private:
#if defined(BLITZAR_HAS_MPI)
    MpiCommunicator communicator_{MPI_COMM_WORLD};
    bool registered_{false};
#endif
    int rank_{0};
    int size_{1};
    blitzar_status status_{BLITZAR_STATUS_OK};
};

}  // namespace blitzar_parallel

#endif
