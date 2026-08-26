#ifndef BLITZAR_SDK_CPP_CPP_STATE_HPP
#define BLITZAR_SDK_CPP_CPP_STATE_HPP

#include <blitzar/blitzar.hpp>

namespace blitzar {

struct Context::Impl final {
    struct Deleter final {
        void operator()(blitzar_context* context) const noexcept
        {
            blitzar_context_destroy(context);
        }
    };

    std::unique_ptr<blitzar_context, Deleter> handle;
};

struct Simulation::Impl final {
    struct Deleter final {
        void operator()(blitzar_simulation* simulation) const noexcept
        {
            blitzar_simulation_destroy(simulation);
        }
    };

    std::unique_ptr<blitzar_simulation, Deleter> handle;
};

} // namespace blitzar

#endif
