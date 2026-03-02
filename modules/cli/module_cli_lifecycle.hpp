#pragma once

#include <cstddef>

#include "frontend/FrontendModuleApi.hpp"

namespace grav_module_cli {

class ModuleCliLifecycle final {
public:
    static bool create(
        const grav_module::FrontendModuleHostContextV1 *hostContext,
        void **outModuleState,
        char *errorBuffer,
        std::size_t errorBufferSize);
    static void destroy(void *moduleState);
    static bool start(void *moduleState, char *errorBuffer, std::size_t errorBufferSize);
    static void stop(void *moduleState);
};

} // namespace grav_module_cli
