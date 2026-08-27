#ifndef BLITZAR_MPI_NATIVE_MPI_NATIVE_SESSION_HPP
#define BLITZAR_MPI_NATIVE_MPI_NATIVE_SESSION_HPP

#include "mpi/native/MpiNative.hpp"

#include <blitzar/blitzar.h>
#include <memory>

namespace blitzar_parallel {

class MpiCollectives;
class MpiGhostTransport;
class MpiPacketTransport;

class MpiNativeSession final {
public:
    MpiNativeSession() noexcept;
    ~MpiNativeSession() noexcept;

    MpiNativeSession(const MpiNativeSession&) = delete;
    MpiNativeSession& operator=(const MpiNativeSession&) = delete;
    MpiNativeSession(MpiNativeSession&&) = delete;
    MpiNativeSession& operator=(MpiNativeSession&&) = delete;

    [[nodiscard]] bool IsUsable() const noexcept;
    [[nodiscard]] bool IsDistributed() const noexcept;
    [[nodiscard]] int Rank() const noexcept;
    [[nodiscard]] int Size() const noexcept;
    [[nodiscard]] blitzar_status Status() const noexcept;

private:
    friend class MpiCollectives;
    friend class MpiGhostTransport;
    friend class MpiPacketTransport;

    struct Impl;

    [[nodiscard]] const MpiNative& Native() const noexcept;

    std::unique_ptr<Impl> impl_;
    int rank_{0};
    int size_{1};
    blitzar_status status_{BLITZAR_STATUS_OK};
};

} // namespace blitzar_parallel

#endif
