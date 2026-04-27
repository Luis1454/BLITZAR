// File: apps/client-host/client_host_module_ops.hpp
// Purpose: Application entry point or host support for BLITZAR executables.

#ifndef GRAVITY_APPS_MODULE_HOST_MODULE_HOST_MODULE_OPS_HPP_
#define GRAVITY_APPS_MODULE_HOST_MODULE_HOST_MODULE_OPS_HPP_
#include "client/ClientModuleHandle.hpp"
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace grav_client_host {
/// Description: Defines the ClientHostModuleOps data or behavior contract.
class ClientHostModuleOps final {
public:
    /// Description: Describes the build search roots operation contract.
    static std::vector<std::filesystem::path> buildSearchRoots(std::string_view programName);
    static std::string
    /// Description: Describes the resolve module specifier operation contract.
    resolveModuleSpecifier(const std::string& rawSpecifier,
                           const std::vector<std::filesystem::path>& searchRoots);
    /// Description: Describes the expected module id for specifier operation contract.
    static std::string expectedModuleIdForSpecifier(const std::string& rawSpecifier);
    /// Description: Describes the switch module operation contract.
    static bool switchModule(const std::string& moduleSpecifier, const std::string& configPath,
                             const std::vector<std::filesystem::path>& searchRoots,
                             grav_module::ClientModuleHandle& module);
    /// Description: Describes the reload module operation contract.
    static bool reloadModule(const std::string& currentModuleSpecifier,
                             const std::string& configPath,
                             const std::vector<std::filesystem::path>& searchRoots,
                             grav_module::ClientModuleHandle& module);
};
} // namespace grav_client_host
#endif // GRAVITY_APPS_MODULE_HOST_MODULE_HOST_MODULE_OPS_HPP_
