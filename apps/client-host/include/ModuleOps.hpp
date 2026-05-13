/*
 * @file apps/client-host/include/ModuleOps.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Application entry points and host executables for BLITZAR.
 */

#ifndef BLITZAR_APPS_CLIENT_HOST_MODULEOPS_HPP_
#define BLITZAR_APPS_CLIENT_HOST_MODULEOPS_HPP_
#include "client/module/Handle.hpp"
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace bltzr_client_host {
class ClientHostModuleOpsLocal final {
public:
    static std::vector<std::string> moduleFilenameCandidatesForAlias(const std::string& alias);
    static bool isExplicitPath(const std::string& specifier);
    static std::vector<std::filesystem::path> buildSearchRoots(std::string_view programName);
    static std::string resolveModuleSpecifier(const std::string& rawSpecifier,
                                              const std::vector<std::filesystem::path>& searchRoots);
    static std::string expectedModuleIdForSpecifier(const std::string& rawSpecifier);
    static std::string expectedModuleIdForResolvedSpecifier(const std::string& rawSpecifier,
                                                            const std::string& resolvedPath);
    static bool switchModule(const std::string& moduleSpecifier, const std::string& configPath,
                             const std::vector<std::filesystem::path>& searchRoots,
                             bltzr_module::Handle& module);
    static bool reloadModule(const std::string& currentModuleSpecifier,
                             const std::string& configPath,
                             const std::vector<std::filesystem::path>& searchRoots,
                             bltzr_module::Handle& module);
};

class ClientHostModuleOps final {
public:
    static std::vector<std::filesystem::path> buildSearchRoots(std::string_view programName);
    static std::string
    resolveModuleSpecifier(const std::string& rawSpecifier,
                           const std::vector<std::filesystem::path>& searchRoots);
    static std::string expectedModuleIdForSpecifier(const std::string& rawSpecifier);
    static std::string expectedModuleIdForResolvedSpecifier(const std::string& rawSpecifier,
                                                            const std::string& resolvedPath);
    static bool switchModule(const std::string& moduleSpecifier, const std::string& configPath,
                             const std::vector<std::filesystem::path>& searchRoots,
                             bltzr_module::Handle& module);
    static bool reloadModule(const std::string& currentModuleSpecifier,
                             const std::string& configPath,
                             const std::vector<std::filesystem::path>& searchRoots,
                             bltzr_module::Handle& module);
};
} // namespace bltzr_client_host
#endif // BLITZAR_APPS_CLIENT_HOST_MODULEOPS_HPP_
