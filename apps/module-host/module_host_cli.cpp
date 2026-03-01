#include "apps/module-host/module_host_cli.hpp"
#include "frontend/FrontendModuleHandle.hpp"
#include "platform/PlatformPaths.hpp"
#include <algorithm>
#include <cctype>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
namespace grav_module_host {
class ModuleHostCliLocal final {
public:
    static bool parseArgs(int argc, char **argv, HostOptions &outOptions, std::string &outError)
    {
        outOptions = HostOptions{};
        for (int i = 1; i < argc; ++i) {
            const std::string arg(argv[i] ? argv[i] : "");
            if (arg == "--help") {
                outOptions.showHelp = true;
                continue;
            }
            if (arg == "--config" && i + 1 < argc) {
                outOptions.configPath = argv[++i];
                continue;
            }
            if (arg.rfind("--config=", 0) == 0) {
                outOptions.configPath = arg.substr(std::string("--config=").size());
                continue;
            }
            if (arg == "--module" && i + 1 < argc) {
                outOptions.moduleSpecifier = argv[++i];
                continue;
            }
            if (arg.rfind("--module=", 0) == 0) {
                outOptions.moduleSpecifier = arg.substr(std::string("--module=").size());
                continue;
            }
            outError = "unknown argument: " + arg;
            return false;
        }
        return true;
    }
    static void printHelp(std::string_view programName)
    {
        std::cout
            << "Usage: " << programName << " [--config PATH] [--module <alias|path>]\n"
            << "[module-host] commands:\n"
            << "  help\n"
            << "  modules\n"
            << "  module\n"
            << "  reload\n"
            << "  switch <module_alias_or_path>\n"
            << "  quit | exit\n"
            << "  <any other line> -> forwarded to loaded module\n"
            << "[module-host] aliases: cli, gui, echo, qt\n";
    }
    static int run(const HostOptions &options, std::string_view programName)
    {
        const std::filesystem::path executablePath(programName);
        const std::vector<std::filesystem::path> searchRoots = buildSearchRoots(executablePath);
        grav_module::FrontendModuleHandle module{};
        std::string currentModuleSpecifier = options.moduleSpecifier;
        const std::string resolvedInitialModulePath =
            resolveModuleSpecifier(options.moduleSpecifier, searchRoots);
        std::string loadError;
        if (!module.load(resolvedInitialModulePath, options.configPath, loadError)) {
            std::cerr << "[module-host] failed to load module '" << options.moduleSpecifier
                      << "': " << loadError << "\n";
            return 1;
        }
        std::cout << "[module-host] loaded: " << module.moduleName()
                  << " (" << module.loadedPath() << ")\n";
        printHelp(programName);
        bool keepRunning = true;
        std::string line;
        while (keepRunning) {
            std::cout << "module-host> " << std::flush;
            if (!std::getline(std::cin, line)) {
                break;
            }
            const std::string trimmed = trim(line);
            if (trimmed.empty()) {
                continue;
            }
            const std::vector<std::string> tokens = splitTokens(trimmed);
            if (tokens.empty()) {
                continue;
            }
            if (tokens[0] == "help") {
                printHelp(programName);
                continue;
            }
            if (tokens[0] == "modules") {
                std::cout << "[module-host] available aliases:\n";
                std::cout << "  cli  -> " << resolveModuleSpecifier("cli", searchRoots) << "\n";
                std::cout << "  gui  -> " << resolveModuleSpecifier("gui", searchRoots) << "\n";
                std::cout << "  echo -> " << resolveModuleSpecifier("echo", searchRoots) << "\n";
                std::cout << "  qt   -> " << resolveModuleSpecifier("qt", searchRoots) << "\n";
                continue;
            }
            if (tokens[0] == "module") {
                std::cout << "[module-host] current module: " << module.moduleName()
                          << " (" << module.loadedPath() << ")";
                if (!currentModuleSpecifier.empty()) {
                    std::cout << " [specifier=" << currentModuleSpecifier << "]";
                }
                std::cout << "\n";
                continue;
            }
            if (tokens[0] == "quit" || tokens[0] == "exit") {
                break;
            }
            if (tokens[0] == "reload") {
                if (currentModuleSpecifier.empty()) {
                    std::cout << "[module-host] no module specifier to reload\n";
                    continue;
                }
                if (!reloadModule(currentModuleSpecifier, options.configPath, searchRoots, module)) {
                    continue;
                }
                continue;
            }
            if (tokens[0] == "switch") {
                if (tokens.size() < 2u) {
                    std::cout << "[module-host] usage: switch <module_alias_or_path>\n";
                    continue;
                }
                if (!switchModule(tokens[1], options.configPath, searchRoots, module)) {
                    continue;
                }
                currentModuleSpecifier = tokens[1];
                continue;
            }
            bool moduleKeepRunning = true;
            std::string commandError;
            if (!module.handleCommand(trimmed, moduleKeepRunning, commandError)) {
                std::cout << "[module-host] " << commandError << "\n";
                continue;
            }
            if (!moduleKeepRunning) {
                keepRunning = false;
            }
        }
        module.unload();
        return 0;
    }
private:
    static std::string trim(const std::string &input)
    {
        const auto begin = std::find_if_not(input.begin(), input.end(), [](unsigned char c) {
            return std::isspace(c) != 0;
        });
        const auto end = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char c) {
            return std::isspace(c) != 0;
        }).base();
        if (begin >= end) {
            return {};
        }
        return std::string(begin, end);
    }
    static std::vector<std::string> splitTokens(const std::string &line)
    {
        std::vector<std::string> tokens;
        std::string current;
        bool inQuotes = false;
        char quoteChar = '\0';
        for (char c : line) {
            if (!inQuotes && (c == '"' || c == '\'')) {
                inQuotes = true;
                quoteChar = c;
                continue;
            }
            if (inQuotes && c == quoteChar) {
                inQuotes = false;
                quoteChar = '\0';
                continue;
            }
            if (!inQuotes && std::isspace(static_cast<unsigned char>(c)) != 0) {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current.clear();
                }
                continue;
            }
            current.push_back(c);
        }
        if (!current.empty()) {
            tokens.push_back(current);
        }
        return tokens;
    }
    static std::string toLower(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return value;
    }
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
    static std::vector<std::filesystem::path> buildSearchRoots(const std::filesystem::path &exePath)
    {
        std::vector<std::filesystem::path> roots;
        roots.reserve(4u);
        roots.push_back(std::filesystem::current_path());
        if (!exePath.empty()) {
            roots.push_back(exePath.parent_path());
        }
        roots.push_back(std::filesystem::current_path() / "build");
        return roots;
    }
    static std::string resolveModuleSpecifier(
        const std::string &rawSpecifier,
        const std::vector<std::filesystem::path> &searchRoots)
    {
        const std::string specifier = trim(rawSpecifier);
        if (specifier.empty()) return {};
        const std::filesystem::path asPath(specifier);
        const bool explicitPath =
            asPath.is_absolute()
            || (specifier.find('/') != std::string::npos)
            || (specifier.find('\\') != std::string::npos)
            || asPath.has_extension();
        if (explicitPath) return specifier;
        const std::string alias = toLower(specifier);
        const std::vector<std::string> filenames = moduleFilenameCandidatesForAlias(alias);
        if (filenames.empty()) return specifier;
        std::error_code ec;
        for (const auto &root : searchRoots) {
            for (const auto &filename : filenames) {
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
        grav_module::FrontendModuleHandle replacement{};
        std::string switchError;
        const std::string resolvedPath = resolveModuleSpecifier(currentModuleSpecifier, searchRoots);
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
bool ModuleHostCli::parseArgs(int argc, char **argv, HostOptions &outOptions, std::string &outError)
{ return ModuleHostCliLocal::parseArgs(argc, argv, outOptions, outError); }
void ModuleHostCli::printHelp(std::string_view programName)
{ ModuleHostCliLocal::printHelp(programName); }
int ModuleHostCli::run(const HostOptions &options, std::string_view programName)
{ return ModuleHostCliLocal::run(options, programName); }
} // namespace grav_module_host
