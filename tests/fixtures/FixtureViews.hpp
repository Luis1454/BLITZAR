#ifndef BLITZAR_TESTS_FIXTURES_VIEWS_HPP
#define BLITZAR_TESTS_FIXTURES_VIEWS_HPP

#include "core/CoreTypes.hpp"

#include <span>

namespace blitzar_tests {

template <typename State>
[[nodiscard]] blitzar_core::ParticleStateView MakeStateView(const State& state) noexcept
{
    return {state.x.size(), state.x, state.y, state.z, state.velocity_x, state.velocity_y,
        state.velocity_z, state.mass, state.x.size()};
}

template <typename State>
[[nodiscard]] blitzar_core::ParticleOutputView MakeOutputView(State& state) noexcept
{
    return {state.x.size(), state.x, state.y, state.z, state.velocity_x, state.velocity_y,
        state.velocity_z, state.mass};
}

} // namespace blitzar_tests

#endif
