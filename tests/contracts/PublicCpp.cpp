#include <blitzar/blitzar.hpp>
#include <string_view>
#include <type_traits>

static_assert(!std::is_copy_constructible_v<blitzar::Context>);
static_assert(!std::is_copy_assignable_v<blitzar::Simulation>);
static_assert(std::is_same_v<decltype(blitzar::version()), std::string_view>);

int PublicCppCompileProbe()
{
    const blitzar::ParticleInput input{};

    return input.IsSized() ? 0 : 1;
}
