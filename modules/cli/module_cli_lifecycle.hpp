/*
 * @file modules/cli/module_cli_lifecycle.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Command-line client module for runtime control workflows.
 */

#ifndef BLITZAR_MODULES_CLI_MODULE_CLI_LIFECYCLE_HPP_
#define BLITZAR_MODULES_CLI_MODULE_CLI_LIFECYCLE_HPP_
#include "client/ClientModuleApi.hpp"
#include "client/ClientModuleBoundary.hpp"

namespace bltzr_module_cli {
class ModuleCliLifecycle final {
public:
    static bool create(const bltzr_module::ClientHostContextV1* hostContext,
                       const bltzr_module::ClientModuleStateSlot& outModuleState,
                       const bltzr_client::ErrorBufferView& errorBuffer);
    static void destroy(bltzr_module::ClientModuleOpaqueState moduleState);
    static bool start(bltzr_module::ClientModuleOpaqueState moduleState,
                      const bltzr_client::ErrorBufferView& errorBuffer);
    static void stop(bltzr_module::ClientModuleOpaqueState moduleState);
};
} // namespace bltzr_module_cli
#endif // BLITZAR_MODULES_CLI_MODULE_CLI_LIFECYCLE_HPP_
