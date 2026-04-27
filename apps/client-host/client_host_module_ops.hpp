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
    /// Description: Executes the buildSearchRoots operation.
    static std::vector<std::filesystem::path> buildSearchRoots(std::string_view programName);
    static std::string
    resolveModuleSpecifier(const std::string& rawSpecifier,
                           const std::vector<std::filesystem::path>& searchRoots);
    /// Description: Executes the expectedModuleIdForSpecifier operation.
    static std::string expectedModuleIdForSpecifier(const std::string& rawSpecifier);
    static bool switchModule(const std::string& moduleSpecifier, const std::string& configPath,
                             const std::vector<std::filesystem::path>& searchRoots,
                             grav_module::ClientModuleHandle& module);
    static bool reloadModule(const std::string& currentModuleSpecifier,
                             const std::string& configPath,
                             const std::vector<std::filesystem::path>& searchRoots,
                             grav_module::ClientModuleHandle& module);
};
} // namespace grav_client_host
#endif // GRAVITY_APPS_MODULE_HOST_MODULE_HOST_MODULE_OPS_HPP_
