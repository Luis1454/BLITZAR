/*
 * @file apps/client-host/src/ModuleOps.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Application entry points and host executables for BLITZAR.
 */

#include "ModuleOps.hpp"
#include "CliText.hpp"
#include "client/module/Handle.hpp"
#include "platform/Paths.hpp"
#include <algorithm>
#include <iostream>
#include <system_error>
#include <utility>

namespace bltzr_client_host {
std::vector<std::string> ClientHostModuleOpsLocal::moduleFilenameCandidatesForAlias(
    const std::string& alias)
{
    if (alias == "cli")
        return bltzr_platform::sharedLibraryCandidates("blitzarClientModuleCli");
    if (alias == "echo")
        return bltzr_platform::sharedLibraryCandidates("blitzarClientModuleEcho");
    if (alias == "gui")
        return bltzr_platform::sharedLibraryCandidates("blitzarClientModuleGuiProxy");
    if (alias == "qt") {
        std::vector<std::string> candidates =
            bltzr_platform::sharedLibraryCandidates("blitzarClientModuleQtInProc");
        std::vector<std::string> proxyCandidates =
            bltzr_platform::sharedLibraryCandidates("blitzarClientModuleGuiProxy");
        candidates.insert(candidates.end(), proxyCandidates.begin(), proxyCandidates.end());
        return candidates;
    }
    return {};
}

bool ClientHostModuleOpsLocal::isExplicitPath(const std::string& specifier)
{
    const std::filesystem::path asPath(specifier);
    return asPath.is_absolute() || (specifier.find('/') != std::string::npos) ||
           (specifier.find('\\') != std::string::npos) || asPath.has_extension();
}

std::vector<std::filesystem::path> ClientHostModuleOpsLocal::buildSearchRoots(
    std::string_view programName)
{
    std::vector<std::filesystem::path> roots;
    roots.reserve(4u);
    roots.push_back(std::filesystem::current_path());
    const std::filesystem::path executablePath(programName);
    if (!executablePath.empty()) {
        roots.push_back(executablePath.parent_path());
    }
    roots.push_back(std::filesystem::current_path() / "build");
    return roots;
}

std::string ClientHostModuleOpsLocal::resolveModuleSpecifier(
    const std::string& rawSpecifier, const std::vector<std::filesystem::path>& searchRoots)
{
    const std::string specifier = ClientHostCliText::trim(rawSpecifier);
    if (specifier.empty()) {
        return {};
    }
    if (isExplicitPath(specifier)) {
        return specifier;
    }
    const std::string alias = ClientHostCliText::toLower(specifier);
    const std::vector<std::string> filenames = moduleFilenameCandidatesForAlias(alias);
    if (filenames.empty()) {
        return specifier;
    }
    std::error_code ec;
    for (const std::filesystem::path& root : searchRoots)
        for (const std::string& filename : filenames) {
            const std::filesystem::path candidate = root / filename;
            if (std::filesystem::exists(candidate, ec) && !ec) {
                return candidate.string();
            }
        }
    return filenames.front();
}

std::string ClientHostModuleOpsLocal::expectedModuleIdForSpecifier(
    const std::string& rawSpecifier)
{
    const std::string specifier = ClientHostCliText::trim(rawSpecifier);
    if (specifier.empty() || isExplicitPath(specifier)) {
        return {};
    }
    const std::string alias = ClientHostCliText::toLower(specifier);
    return moduleFilenameCandidatesForAlias(alias).empty() ? std::string() : alias;
}

std::string ClientHostModuleOpsLocal::expectedModuleIdForResolvedSpecifier(
    const std::string& rawSpecifier, const std::string& resolvedPath)
{
    const std::string specifier = ClientHostCliText::trim(rawSpecifier);
    if (specifier.empty() || ClientHostModuleOpsLocal::isExplicitPath(specifier)) {
        return {};
    }
    const std::string alias = ClientHostCliText::toLower(specifier);
    if (alias != "qt") {
        return expectedModuleIdForSpecifier(specifier);
    }
    const std::filesystem::path path(resolvedPath);
    const std::string filename = path.filename().string();
    const std::vector<std::string> guiCandidates =
        bltzr_platform::sharedLibraryCandidates("blitzarClientModuleGuiProxy");
    if (std::find(guiCandidates.begin(), guiCandidates.end(), filename) != guiCandidates.end()) {
        std::cout << "[client-host] Qt module absent; using GUI proxy module\n";
        return "gui";
    }
    return "qt";
}

bool ClientHostModuleOpsLocal::switchModule(const std::string& moduleSpecifier,
                                             const std::string& configPath,
                                             const std::vector<std::filesystem::path>& searchRoots,
                                             bltzr_module::Handle& module)
{
    const std::string resolvedPath = resolveModuleSpecifier(moduleSpecifier, searchRoots);
    const std::string expectedModuleId =
        ClientHostModuleOpsLocal::expectedModuleIdForResolvedSpecifier(moduleSpecifier,
                                                                       resolvedPath);
    bltzr_module::Handle replacement{};
    std::string switchError;
    if (!replacement.load(resolvedPath, configPath, expectedModuleId, switchError)) {
        std::cout << "[client-host] switch failed: " << switchError << "\n";
        return false;
    }
    module = std::move(replacement);
    std::cout << "[client-host] switched to: " << module.moduleName() << " ("
              << module.loadedPath() << ")\n";
    return true;
}

bool ClientHostModuleOpsLocal::reloadModule(const std::string& currentModuleSpecifier,
                                             const std::string& configPath,
                                             const std::vector<std::filesystem::path>& searchRoots,
                                             bltzr_module::Handle& module)
{
    const std::string resolvedPath = resolveModuleSpecifier(currentModuleSpecifier, searchRoots);
    const std::string expectedModuleId =
        ClientHostModuleOpsLocal::expectedModuleIdForResolvedSpecifier(currentModuleSpecifier,
                                                                       resolvedPath);
    bltzr_module::Handle replacement{};
    std::string switchError;
    if (!replacement.load(resolvedPath, configPath, expectedModuleId, switchError)) {
        std::cout << "[client-host] reload failed: " << switchError << "\n";
        return false;
    }
    module = std::move(replacement);
    std::cout << "[client-host] reloaded: " << module.moduleName() << " ("
              << module.loadedPath() << ")\n";
    return true;
}

std::vector<std::filesystem::path>
ClientHostModuleOps::buildSearchRoots(std::string_view programName)
{
    return ClientHostModuleOpsLocal::buildSearchRoots(programName);
}

std::string
ClientHostModuleOps::resolveModuleSpecifier(const std::string& rawSpecifier,
                                            const std::vector<std::filesystem::path>& searchRoots)
{
    return ClientHostModuleOpsLocal::resolveModuleSpecifier(rawSpecifier, searchRoots);
}

std::string ClientHostModuleOps::expectedModuleIdForSpecifier(const std::string& rawSpecifier)
{
    return ClientHostModuleOpsLocal::expectedModuleIdForSpecifier(rawSpecifier);
}

std::string ClientHostModuleOps::expectedModuleIdForResolvedSpecifier(
    const std::string& rawSpecifier, const std::string& resolvedPath)
{
    return ClientHostModuleOpsLocal::expectedModuleIdForResolvedSpecifier(rawSpecifier,
                                                                         resolvedPath);
}

bool ClientHostModuleOps::switchModule(const std::string& moduleSpecifier,
                                       const std::string& configPath,
                                       const std::vector<std::filesystem::path>& searchRoots,
                                       bltzr_module::Handle& module)
{
    return ClientHostModuleOpsLocal::switchModule(moduleSpecifier, configPath, searchRoots, module);
}

bool ClientHostModuleOps::reloadModule(const std::string& currentModuleSpecifier,
                                       const std::string& configPath,
                                       const std::vector<std::filesystem::path>& searchRoots,
                                       bltzr_module::Handle& module)
{
    return ClientHostModuleOpsLocal::reloadModule(currentModuleSpecifier, configPath, searchRoots,
                                                  module);
}
} // namespace bltzr_client_host
