#include "apps/client-host/client_host_module_ops.hpp"
#include "client/ClientModuleHandle.hpp"
#include "platform/PlatformPaths.hpp"
namespace grav_client_host {
class ClientHostModuleOpsLocal final {
#include "apps/client-host/client_host_cli_text.hpp"
#include <iostream>
#include <system_error>
#include <utility>
public:
    static std::vector<std::string> moduleFilenameCandidatesForAlias(const std::string& alias)
    {
        if (alias == "cli")
            return grav_platform::sharedLibraryCandidates("gravityClientModuleCli");
        if (alias == "echo")
            return grav_platform::sharedLibraryCandidates("gravityClientModuleEcho");
        if (alias == "gui")
            return grav_platform::sharedLibraryCandidates("gravityClientModuleGuiProxy");
        if (alias == "qt")
            return grav_platform::sharedLibraryCandidates("gravityClientModuleQtInProc");
        return {};
    }
    static bool isExplicitPath(const std::string& specifier)
    {
        const std::filesystem::path asPath(specifier);
        return asPath.is_absolute() || (specifier.find('/') != std::string::npos) ||
               (specifier.find('\\') != std::string::npos) || asPath.has_extension();
    }
    static std::vector<std::filesystem::path> buildSearchRoots(std::string_view programName)
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
    static std::string resolveModuleSpecifier(const std::string& rawSpecifier,
                                              const std::vector<std::filesystem::path>& searchRoots)
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
    static std::string expectedModuleIdForSpecifier(const std::string& rawSpecifier)
    {
        const std::string specifier = ClientHostCliText::trim(rawSpecifier);
        if (specifier.empty() || isExplicitPath(specifier)) {
            return {};
        }
        const std::string alias = ClientHostCliText::toLower(specifier);
        return moduleFilenameCandidatesForAlias(alias).empty() ? std::string() : alias;
    }
    static bool switchModule(const std::string& moduleSpecifier, const std::string& configPath,
                             const std::vector<std::filesystem::path>& searchRoots,
                             grav_module::ClientModuleHandle& module)
    {
        const std::string resolvedPath = resolveModuleSpecifier(moduleSpecifier, searchRoots);
        const std::string expectedModuleId = expectedModuleIdForSpecifier(moduleSpecifier);
        grav_module::ClientModuleHandle replacement{};
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
    static bool reloadModule(const std::string& currentModuleSpecifier,
                             const std::string& configPath,
                             const std::vector<std::filesystem::path>& searchRoots,
                             grav_module::ClientModuleHandle& module)
    {
        const std::string resolvedPath =
            resolveModuleSpecifier(currentModuleSpecifier, searchRoots);
        const std::string expectedModuleId = expectedModuleIdForSpecifier(currentModuleSpecifier);
        grav_module::ClientModuleHandle replacement{};
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
};
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
bool ClientHostModuleOps::switchModule(const std::string& moduleSpecifier,
                                       const std::string& configPath,
                                       const std::vector<std::filesystem::path>& searchRoots,
                                       grav_module::ClientModuleHandle& module)
{
    return ClientHostModuleOpsLocal::switchModule(moduleSpecifier, configPath, searchRoots, module);
}
bool ClientHostModuleOps::reloadModule(const std::string& currentModuleSpecifier,
                                       const std::string& configPath,
                                       const std::vector<std::filesystem::path>& searchRoots,
                                       grav_module::ClientModuleHandle& module)
{
    return ClientHostModuleOpsLocal::reloadModule(currentModuleSpecifier, configPath, searchRoots,
                                                  module);
}
} // namespace grav_client_host
