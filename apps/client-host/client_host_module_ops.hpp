/*
 * @file apps/client-host/client_host_module_ops.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Application entry points and host executables for BLITZAR.
 */

#ifndef BLITZAR_APPS_MODULE_HOST_MODULE_HOST_MODULE_OPS_HPP_
#define BLITZAR_APPS_MODULE_HOST_MODULE_HOST_MODULE_OPS_HPP_
#include "client/ClientModuleHandle.hpp"
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace bltzr_client_host {
class ClientHostModuleOps final {
public:
    static std::vector<std::filesystem::path> buildSearchRoots(std::string_view programName);
    static std::string
    resolveModuleSpecifier(const std::string& rawSpecifier,
                           const std::vector<std::filesystem::path>& searchRoots);
    static std::string expectedModuleIdForSpecifier(const std::string& rawSpecifier);
    static bool switchModule(const std::string& moduleSpecifier, const std::string& configPath,
                             const std::vector<std::filesystem::path>& searchRoots,
                             bltzr_module::ClientModuleHandle& module);
    static bool reloadModule(const std::string& currentModuleSpecifier,
                             const std::string& configPath,
                             const std::vector<std::filesystem::path>& searchRoots,
                             bltzr_module::ClientModuleHandle& module);
};
} // namespace bltzr_client_host
#endif // BLITZAR_APPS_MODULE_HOST_MODULE_HOST_MODULE_OPS_HPP_
