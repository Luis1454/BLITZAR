#include <iostream>
#include <system_error>
#include <utility>

#include "apps/module-host/module_host_cli_text.hpp"
#include "apps/module-host/module_host_module_ops.hpp"
#include "frontend/FrontendModuleHandle.hpp"
#include "platform/PlatformPaths.hpp"

namespace grav_module_host {

class ModuleHostModuleOpsLocal final {
public:
    static std::vector<std::string> moduleFilenameCandidatesForAlias(const std::string &alias)
    {
        if (alias == "cli") {
            return grav_platform::sharedLibraryCandidates("gravityFrontendModuleCli");
        }
        if (alias == "echo") {
            return grav_platform::sharedLibraryCandidates("gravityFrontendModuleEcho");
        }
        if (alias == "gui") {
            return grav_platform::sharedLibraryCandidates("gravityFrontendModuleGuiProxy");
        }
        if (alias == "qt") {
            return grav_platform::sharedLibraryCandidates("gravityFrontendModuleQtInProc");
        }
        return {};
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

    static std::string resolveModuleSpecifier(
        const std::string &rawSpecifier,
        const std::vector<std::filesystem::path> &searchRoots)
    {
        const std::string specifier = ModuleHostCliText::trim(rawSpecifier);
        if (specifier.empty()) {
            return {};
        }

        const std::filesystem::path asPath(specifier);
        const bool explicitPath =
            asPath.is_absolute()
            || (specifier.find('/') != std::string::npos)
            || (specifier.find('\\') != std::string::npos)
            || asPath.has_extension();
        if (explicitPath) {
            return specifier;
        }

        const std::string alias = ModuleHostCliText::toLower(specifier);
        const std::vector<std::string> filenames = moduleFilenameCandidatesForAlias(alias);
        if (filenames.empty()) {
            return specifier;
        }

        std::error_code ec;
        for (const std::filesystem::path &root : searchRoots) {
            for (const std::string &filename : filenames) {
                const std::filesystem::path candidate = root / filename;
                if (std::filesystem::exists(candidate, ec) && !ec) {
                    return candidate.string();
                }
            }
        }
        return filenames.front();
    }

    static bool switchModule(
        const std::string &moduleSpecifier,
        const std::string &configPath,
        const std::vector<std::filesystem::path> &searchRoots,
        grav_module::FrontendModuleHandle &module)
    {
        const std::string resolvedPath = resolveModuleSpecifier(moduleSpecifier, searchRoots);
        grav_module::FrontendModuleHandle replacement{};
        std::string switchError;
        if (!replacement.load(resolvedPath, configPath, switchError)) {
            std::cout << "[module-host] switch failed: " << switchError << "\n";
            return false;
        }
        module = std::move(replacement);
        std::cout << "[module-host] switched to: " << module.moduleName()
                  << " (" << module.loadedPath() << ")\n";
        return true;
    }

    static bool reloadModule(
        const std::string &currentModuleSpecifier,
        const std::string &configPath,
        const std::vector<std::filesystem::path> &searchRoots,
        grav_module::FrontendModuleHandle &module)
    {
        const std::string resolvedPath = resolveModuleSpecifier(currentModuleSpecifier, searchRoots);
        grav_module::FrontendModuleHandle replacement{};
        std::string switchError;
        if (!replacement.load(resolvedPath, configPath, switchError)) {
            std::cout << "[module-host] reload failed: " << switchError << "\n";
            return false;
        }
        module = std::move(replacement);
        std::cout << "[module-host] reloaded: " << module.moduleName()
                  << " (" << module.loadedPath() << ")\n";
        return true;
    }
};

std::vector<std::filesystem::path> ModuleHostModuleOps::buildSearchRoots(std::string_view programName)
{
    return ModuleHostModuleOpsLocal::buildSearchRoots(programName);
}

std::string ModuleHostModuleOps::resolveModuleSpecifier(
    const std::string &rawSpecifier,
    const std::vector<std::filesystem::path> &searchRoots)
{
    return ModuleHostModuleOpsLocal::resolveModuleSpecifier(rawSpecifier, searchRoots);
}

bool ModuleHostModuleOps::switchModule(
    const std::string &moduleSpecifier,
    const std::string &configPath,
    const std::vector<std::filesystem::path> &searchRoots,
    grav_module::FrontendModuleHandle &module)
{
    return ModuleHostModuleOpsLocal::switchModule(moduleSpecifier, configPath, searchRoots, module);
}

bool ModuleHostModuleOps::reloadModule(
    const std::string &currentModuleSpecifier,
    const std::string &configPath,
    const std::vector<std::filesystem::path> &searchRoots,
    grav_module::FrontendModuleHandle &module)
{
    return ModuleHostModuleOpsLocal::reloadModule(currentModuleSpecifier, configPath, searchRoots, module);
}

} // namespace grav_module_host
