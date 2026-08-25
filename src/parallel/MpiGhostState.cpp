#include "parallel/MpiGhostState.hpp"

#include "parallel/MpiGhostProtocol.hpp"
#include "parallel/MpiGhostTransport.hpp"

#include <utility>

#if defined(BLITZAR_HAS_MPI)
#include <mpi.h>
#endif

namespace blitzar_parallel {

void MpiGhostTransport::ClearExchange(MpiGhostExchange::Impl& state) noexcept
{
    state.active = false;
#if defined(BLITZAR_HAS_MPI)

    state.local_wire.clear();
    state.receive_wire.clear();
    state.receive_requests.clear();
    state.send_requests.clear();
    state.receive_statuses.clear();
    state.receive_chunks.clear();
#endif
}

void MpiGhostTransport::AbortExchange(MpiGhostExchange::Impl& state) noexcept
{
#if defined(BLITZAR_HAS_MPI)
    if (state.active) {
        int initialized = 0;
        int finalized = 0;

        if (MPI_Initialized(&initialized) == MPI_SUCCESS && initialized != 0 &&
            MPI_Finalized(&finalized) == MPI_SUCCESS && finalized == 0) {
            MpiGhostProtocol::CancelRequests(state.receive_requests);
            MpiGhostProtocol::CancelRequests(state.send_requests);
        }
        else {
            state.receive_requests.clear();
            state.send_requests.clear();
        }
    }
#endif

    ClearExchange(state);

    state.transfer = {};
}

MpiGhostExchange::MpiGhostExchange() noexcept = default;

MpiGhostExchange::~MpiGhostExchange() noexcept
{
    if (impl_ != nullptr) {
        MpiGhostTransport::AbortExchange(*impl_);
    }
}

MpiGhostExchange::MpiGhostExchange(MpiGhostExchange&& other) noexcept = default;

MpiGhostExchange& MpiGhostExchange::operator=(MpiGhostExchange&& other) noexcept
{
    if (this == &other) {
        return *this;
    }

    if (impl_ != nullptr) {
        MpiGhostTransport::AbortExchange(*impl_);
    }

    impl_ = std::move(other.impl_);

    return *this;
}

MpiGhostExchange::TransferStats MpiGhostExchange::Transfer() const noexcept
{
    return impl_ == nullptr ? TransferStats{} : impl_->transfer;
}

} // namespace blitzar_parallel
