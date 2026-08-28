#ifndef BLITZAR_TESTS_LAYOUT_LAYOUT_STATE_HPP
#define BLITZAR_TESTS_LAYOUT_LAYOUT_STATE_HPP

#include "core/CoreTypes.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace blitzar_layout {

class LayoutState final {
public:
    LayoutState(std::size_t count, std::uint64_t seed);

    [[nodiscard]] std::size_t Count() const noexcept;
    [[nodiscard]] blitzar_core::ParticleStateView View() const noexcept;
    [[nodiscard]] blitzar_core::Vector3 Minimum() const noexcept;
    [[nodiscard]] blitzar_core::Vector3 Maximum() const noexcept;
    [[nodiscard]] std::uint64_t Hash() const noexcept;

private:
    std::size_t count_{};
    std::vector<blitzar_core::Scalar> x_;
    std::vector<blitzar_core::Scalar> y_;
    std::vector<blitzar_core::Scalar> z_;
    std::vector<blitzar_core::Scalar> velocity_x_;
    std::vector<blitzar_core::Scalar> velocity_y_;
    std::vector<blitzar_core::Scalar> velocity_z_;
    std::vector<blitzar_core::Scalar> mass_;
};

} // namespace blitzar_layout

#endif
