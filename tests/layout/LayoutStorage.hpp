#ifndef BLITZAR_TESTS_LAYOUT_LAYOUT_STORAGE_HPP
#define BLITZAR_TESTS_LAYOUT_LAYOUT_STORAGE_HPP

#include "core/CoreTypes.hpp"
#include "layout/LayoutState.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace blitzar_layout {

enum class LayoutKind : std::uint8_t { Soa, Aosoa };

struct LayoutSpec final {
    LayoutKind kind{};
    std::size_t tile_width{};
};

class LayoutStorage final {
public:
    LayoutStorage(std::size_t count, LayoutKind kind, std::size_t tile_width);

    [[nodiscard]] bool IsValid() const noexcept;
    [[nodiscard]] bool Load(const LayoutState& state, std::span<const std::size_t> order) noexcept;
    [[nodiscard]] blitzar_core::ParticleStateView View() const noexcept;
    [[nodiscard]] double Scan() const noexcept;
    [[nodiscard]] std::size_t CandidateBytes() const noexcept;
    [[nodiscard]] std::size_t MaterializedBytes() const noexcept;
    [[nodiscard]] std::size_t CacheLineVisits() const noexcept;
    [[nodiscard]] std::uint64_t LogicalHash() const noexcept;
    [[nodiscard]] std::uint64_t ByteHash() const noexcept;

private:
    [[nodiscard]] blitzar_core::Scalar CandidateValue(
        std::size_t field, std::size_t index) const noexcept;
    void StoreCandidate(std::size_t field, std::size_t index, blitzar_core::Scalar value) noexcept;

    static constexpr std::size_t FieldCount = 7;
    static constexpr std::size_t CacheLineBytes = 64;

    std::size_t count_{};
    LayoutKind kind_{};
    std::size_t tile_width_{};
    std::vector<blitzar_core::Scalar> tiled_;
    std::array<std::vector<blitzar_core::Scalar>, FieldCount> fields_;
};

} // namespace blitzar_layout

#endif
