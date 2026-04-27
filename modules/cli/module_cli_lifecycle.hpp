// File: modules/cli/module_cli_lifecycle.hpp
// Purpose: Client module implementation for BLITZAR extension workflows.

#ifndef GRAVITY_MODULES_CLI_MODULE_CLI_LIFECYCLE_HPP_
#define GRAVITY_MODULES_CLI_MODULE_CLI_LIFECYCLE_HPP_
#include "client/ClientModuleApi.hpp"
#include "client/ClientModuleBoundary.hpp"
namespace grav_module_cli {
/// Description: Defines the ModuleCliLifecycle data or behavior contract.
class ModuleCliLifecycle final {
public:
    static bool create(const grav_module::ClientHostContextV1* hostContext,
                       const grav_module::ClientModuleStateSlot& outModuleState,
                       const grav_client::ErrorBufferView& errorBuffer);
    /// Description: Executes the destroy operation.
    static void destroy(grav_module::ClientModuleOpaqueState moduleState);
    static bool start(grav_module::ClientModuleOpaqueState moduleState,
                      const grav_client::ErrorBufferView& errorBuffer);
    /// Description: Executes the stop operation.
    static void stop(grav_module::ClientModuleOpaqueState moduleState);
};
} // namespace grav_module_cli
#endif // GRAVITY_MODULES_CLI_MODULE_CLI_LIFECYCLE_HPP_
