#ifndef BLITZAR_PARALLEL_MPI_NATIVE_MPI_NATIVE_HPP
#define BLITZAR_PARALLEL_MPI_NATIVE_MPI_NATIVE_HPP

#include "core/contracts/Types.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace blitzar_parallel {

struct NativeByteAllToAllRequest final {
    std::span<const std::byte> send_wire;
    std::span<const int> send_bytes;
    std::span<const int> send_offsets;
    std::span<std::byte> receive_wire;
    std::span<const int> receive_bytes;
    std::span<const int> receive_offsets;
};

struct NativeByteAllGatherRequest final {
    std::span<const std::byte> send_wire;
    int send_bytes{};
    std::span<std::byte> receive_wire;
    std::span<const int> receive_bytes;
    std::span<const int> receive_offsets;
};

struct NativeGhostReceiveRequest final {
    std::span<std::byte> wire;
    std::size_t offset_bytes{};
    int bytes{};
    int peer{};
    int tag{};
};

struct NativeGhostSendRequest final {
    std::span<const std::byte> wire;
    std::size_t offset_bytes{};
    int bytes{};
    int peer{};
    int tag{};
};

class MpiNativeGhost final {
public:
    MpiNativeGhost() noexcept;
    ~MpiNativeGhost() noexcept;

    MpiNativeGhost(const MpiNativeGhost&) = delete;
    MpiNativeGhost& operator=(const MpiNativeGhost&) = delete;
    MpiNativeGhost(MpiNativeGhost&&) = delete;
    MpiNativeGhost& operator=(MpiNativeGhost&&) = delete;

    void Reset() noexcept;
    void Cancel() noexcept;

private:
    struct Impl;

    friend class MpiNative;

    std::unique_ptr<Impl> impl_;
};

class MpiNative final {
public:
    MpiNative() noexcept;
    ~MpiNative() noexcept;

    MpiNative(const MpiNative&) = delete;
    MpiNative& operator=(const MpiNative&) = delete;
    MpiNative(MpiNative&&) = delete;
    MpiNative& operator=(MpiNative&&) = delete;

    [[nodiscard]] bool IsUsable() const noexcept;
    [[nodiscard]] bool IsDistributed() const noexcept;
    [[nodiscard]] int Rank() const noexcept;
    [[nodiscard]] int Size() const noexcept;
    [[nodiscard]] blitzar_status Status() const noexcept;

    [[nodiscard]] blitzar_status ReduceMaxInt(int local_value, int& global_value) const noexcept;
    [[nodiscard]] blitzar_status ReduceBounds(std::span<blitzar_core::Scalar> minimum,
        std::span<blitzar_core::Scalar> maximum) const noexcept;
    [[nodiscard]] blitzar_status BroadcastScalars(
        std::span<blitzar_core::Scalar> values, int root) const noexcept;
    [[nodiscard]] blitzar_status BroadcastIds(
        std::span<std::uint64_t> values, int root) const noexcept;

    [[nodiscard]] blitzar_status AllToAllCounts(
        std::span<const int> send_counts, std::span<int> receive_counts) const noexcept;
    [[nodiscard]] blitzar_status AllGatherCounts(
        int local_count, std::span<int> counts) const noexcept;
    [[nodiscard]] blitzar_status AllToAllBytes(
        const NativeByteAllToAllRequest& request) const noexcept;
    [[nodiscard]] blitzar_status AllGatherBytes(
        const NativeByteAllGatherRequest& request) const noexcept;

    [[nodiscard]] blitzar_status ReserveGhost(MpiNativeGhost& ghost, std::size_t receive_capacity,
        std::size_t send_capacity) const noexcept;
    [[nodiscard]] blitzar_status ResizeGhost(
        MpiNativeGhost& ghost, std::size_t receive_count, std::size_t send_count) const noexcept;
    [[nodiscard]] blitzar_status PostGhostReceive(
        MpiNativeGhost& ghost, const NativeGhostReceiveRequest& request) const noexcept;
    [[nodiscard]] blitzar_status PostGhostSend(
        MpiNativeGhost& ghost, const NativeGhostSendRequest& request) const noexcept;
    [[nodiscard]] blitzar_status WaitGhost(
        MpiNativeGhost& ghost, std::span<std::size_t> receive_bytes) const noexcept;

private:
    struct Impl;

    [[nodiscard]] blitzar_status Initialize() noexcept;
    void Release() noexcept;

    std::unique_ptr<Impl> impl_;
    int rank_{0};
    int size_{1};
    blitzar_status status_{BLITZAR_STATUS_OK};
};

} // namespace blitzar_parallel

#endif
