#ifndef BLITZAR_PARALLEL_MPI_SESSION_HPP
#define BLITZAR_PARALLEL_MPI_SESSION_HPP

#include <blitzar/blitzar.h>
#include <memory>

namespace blitzar_parallel {

class MpiCollectives;
class MpiGhostTransport;
class MpiPacketTransport;

class MpiSession final {
public:
    MpiSession() noexcept;
    ~MpiSession() noexcept;

    MpiSession(const MpiSession&) = delete;
    MpiSession& operator=(const MpiSession&) = delete;
    MpiSession(MpiSession&&) = delete;
    MpiSession& operator=(MpiSession&&) = delete;

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

    [[nodiscard]] blitzar_status InitializeMpi() noexcept;
    void ReleaseMpi() noexcept;
    [[nodiscard]] blitzar_status ReadCommunicator() noexcept;
    [[nodiscard]] const Impl& Native() const noexcept;

    std::unique_ptr<Impl> impl_;
    int rank_{0};
    int size_{1};
    blitzar_status status_{BLITZAR_STATUS_OK};
};

} // namespace blitzar_parallel

#endif
