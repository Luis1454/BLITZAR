#ifndef BLITZAR_TESTS_LAYOUT_LAYOUT_ORDER_HPP
#define BLITZAR_TESTS_LAYOUT_LAYOUT_ORDER_HPP

#include "layout/LayoutState.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace blitzar_layout {

enum class OrderKind : std::uint8_t { StableComparison, StableRadix };

class LayoutOrder final {
public:
    explicit LayoutOrder(std::size_t count);

    void Build(const LayoutState& state, OrderKind kind) noexcept;
    [[nodiscard]] std::span<const std::size_t> Values() const noexcept;
    [[nodiscard]] std::span<const std::uint64_t> Keys() const noexcept;
    [[nodiscard]] bool IsStable() const noexcept;
    [[nodiscard]] bool Matches(const LayoutOrder& other) const noexcept;
    [[nodiscard]] std::uint64_t Hash() const noexcept;
    [[nodiscard]] double Locality(const LayoutState& state) const noexcept;

private:
    void BuildKeys(const LayoutState& state) noexcept;
    void BuildComparison() noexcept;
    void BuildRadix() noexcept;

    std::vector<std::size_t> order_;
    std::vector<std::size_t> scratch_;
    std::vector<std::uint64_t> keys_;
};

} // namespace blitzar_layout

#endif
