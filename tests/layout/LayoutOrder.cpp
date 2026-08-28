#include "layout/LayoutOrder.hpp"

#include "trees/octree/OctreeMorton.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <numeric>

namespace blitzar_layout {

LayoutOrder::LayoutOrder(std::size_t count)
{
    order_.resize(count);
    scratch_.resize(count);
    keys_.resize(count);
}

void LayoutOrder::Build(const LayoutState& state, OrderKind kind) noexcept
{
    BuildKeys(state);
    std::iota(order_.begin(), order_.end(), std::size_t{0});

    if (kind == OrderKind::StableRadix) {
        BuildRadix();
    }
    else {
        BuildComparison();
    }
}

void LayoutOrder::BuildKeys(const LayoutState& state) noexcept
{
    const auto view = state.View();
    const auto minimum = state.Minimum();
    const auto maximum = state.Maximum();

    for (std::size_t index = 0; index < order_.size(); ++index) {
        keys_[index] = blitzar_tree_ordering::MortonKey(
            {view.x[index], view.y[index], view.z[index]}, minimum, maximum);
    }
}

void LayoutOrder::BuildComparison() noexcept
{
    std::sort(
        order_.begin(), order_.end(), [this](const std::size_t left, const std::size_t right) {
            return keys_[left] < keys_[right] || (keys_[left] == keys_[right] && left < right);
        });
}

void LayoutOrder::BuildRadix() noexcept
{
    for (std::size_t pass = 0; pass < 8; ++pass) {
        std::array<std::size_t, 256> offsets{};

        for (const std::size_t index : order_) {
            const std::size_t bucket = (keys_[index] >> (pass * 8U)) & 0xffU;

            ++offsets[bucket];
        }

        std::size_t begin = 0;

        for (std::size_t bucket = 0; bucket < offsets.size(); ++bucket) {
            const std::size_t count = offsets[bucket];

            offsets[bucket] = begin;
            begin += count;
        }

        for (const std::size_t index : order_) {
            const std::size_t bucket = (keys_[index] >> (pass * 8U)) & 0xffU;

            scratch_[offsets[bucket]++] = index;
        }

        order_.swap(scratch_);
    }
}

std::span<const std::size_t> LayoutOrder::Values() const noexcept
{
    return order_;
}

std::span<const std::uint64_t> LayoutOrder::Keys() const noexcept
{
    return keys_;
}

bool LayoutOrder::IsStable() const noexcept
{
    for (std::size_t position = 1; position < order_.size(); ++position) {
        const std::size_t previous = order_[position - 1];
        const std::size_t current = order_[position];

        if (keys_[previous] > keys_[current] ||
            (keys_[previous] == keys_[current] && previous >= current)) {
            return false;
        }
    }

    return true;
}

bool LayoutOrder::Matches(const LayoutOrder& other) const noexcept
{
    return order_ == other.order_ && keys_ == other.keys_;
}

std::uint64_t LayoutOrder::Hash() const noexcept
{
    std::uint64_t hash = 1469598103934665603ULL;

    for (const std::size_t index : order_) {
        hash ^= static_cast<std::uint64_t>(index);
        hash *= 1099511628211ULL;
        hash ^= keys_[index];
        hash *= 1099511628211ULL;
    }

    return hash;
}

double LayoutOrder::Locality(const LayoutState& state) const noexcept
{
    if (order_.size() < 2) {
        return 0.0;
    }

    const auto view = state.View();
    double distance = 0.0;

    for (std::size_t position = 1; position < order_.size(); ++position) {
        const std::size_t previous = order_[position - 1];
        const std::size_t current = order_[position];
        const double x = view.x[current] - view.x[previous];
        const double y = view.y[current] - view.y[previous];
        const double z = view.z[current] - view.z[previous];

        distance += x * x + y * y + z * z;
    }

    return distance / static_cast<double>(order_.size() - 1);
}

} // namespace blitzar_layout
