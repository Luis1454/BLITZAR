#pragma once

#include "frontend/FrontendModuleApi.hpp"
#include "frontend/FrontendModuleBoundary.hpp"

namespace grav_module_cli {

class ModuleCliLifecycle final {
public:
    static bool create(
        const grav_module::FrontendModuleHostContextV1 *hostContext,
        const grav_module::FrontendModuleStateSlot &outModuleState,
        const grav_frontend::ErrorBufferView &errorBuffer);
    static void destroy(grav_module::FrontendModuleOpaqueState moduleState);
    static bool start(
        grav_module::FrontendModuleOpaqueState moduleState,
        const grav_frontend::ErrorBufferView &errorBuffer);
    static void stop(grav_module::FrontendModuleOpaqueState moduleState);
};

} // namespace grav_module_cli
