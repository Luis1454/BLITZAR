/*
 * @file modules/cli/Lifecycle.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Command-line client module for runtime control workflows.
 */

#ifndef BLITZAR_MODULES_CLI_LIFECYCLE_HPP_
#define BLITZAR_MODULES_CLI_LIFECYCLE_HPP_
#include "client/module/Api.hpp"
#include "client/module/Boundary.hpp"

namespace bltzr_module_cli {

class Lifecycle final {
public:
    static bool create(const bltzr_module::HostContextV1* hostContext,
                       const bltzr_module::StateSlot& outModuleState,
                       const bltzr_client::ErrorBufferView& errorBuffer);

    static void destroy(bltzr_module::OpaqueState moduleState);

    static bool start(bltzr_module::OpaqueState moduleState,
                      const bltzr_client::ErrorBufferView& errorBuffer);

    static void stop(bltzr_module::OpaqueState moduleState);
};

} // namespace bltzr_module_cli
#endif // BLITZAR_MODULES_CLI_LIFECYCLE_HPP_
