#include "sim/FrontendModuleHandle.hpp"
#include "sim/PlatformPaths.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::string trim(const std::string &input)
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

std::vector<std::string> splitTokens(const std::string &line)
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

std::string defaultModulePath()
{
    return "cli";
}

std::vector<std::string> moduleFilenameCandidatesForAlias(const std::string &alias)
{
    if (alias == "cli") {
        return sim::platform::sharedLibraryCandidates("gravityFrontendModuleCli");
    }
    if (alias == "echo") {
        return sim::platform::sharedLibraryCandidates("gravityFrontendModuleEcho");
    }
    if (alias == "gui") {
        return sim::platform::sharedLibraryCandidates("gravityFrontendModuleGuiProxy");
    }
    if (alias == "qt") {
        return sim::platform::sharedLibraryCandidates("gravityFrontendModuleQtInProc");
    }
    return {};
}

std::string toLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::vector<std::filesystem::path> buildSearchRoots(const std::filesystem::path &exePath)
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

std::string resolveModuleSpecifier(
    const std::string &rawSpecifier,
    const std::vector<std::filesystem::path> &searchRoots)
{
    const std::string specifier = trim(rawSpecifier);
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

    const std::string alias = toLower(specifier);
    const std::vector<std::string> filenames = moduleFilenameCandidatesForAlias(alias);
    if (filenames.empty()) {
        return specifier;
    }

    std::error_code ec;
    for (const auto &root : searchRoots) {
        for (const auto &filename : filenames) {
            const std::filesystem::path candidate = root / filename;
            if (std::filesystem::exists(candidate, ec) && !ec) {
                return candidate.string();
            }
        }
    }

    // Fall back to first candidate in PATH lookup / current loader rules.
    return filenames.front();
}

bool loadModule(
    const std::string &modulePath,
    const std::string &configPath,
    sim::module::FrontendModuleHandle &outModule,
    std::string &outError)
{
    return outModule.load(modulePath, configPath, outError);
}

void printHelp()
{
    std::cout
        << "[module-host] commands:\n"
        << "  help\n"
        << "  modules\n"
        << "  module\n"
        << "  reload\n"
        << "  switch <module_alias_or_path>\n"
        << "  quit | exit\n"
        << "  <any other line> -> forwarded to loaded module\n";
}

} // namespace

int main(int argc, char **argv)
{
    try {
    std::string configPath = "simulation.ini";
    std::string initialModulePath = defaultModulePath();
    bool showHelp = false;
    const std::filesystem::path executablePath = (argc > 0 && argv[0] != nullptr)
        ? std::filesystem::path(argv[0])
        : std::filesystem::path();
    const std::vector<std::filesystem::path> searchRoots = buildSearchRoots(executablePath);

    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i] ? argv[i] : "");
        if (arg == "--help") {
            showHelp = true;
            continue;
        }
        if (arg == "--config" && i + 1 < argc) {
            configPath = argv[++i];
            continue;
        }
        if (arg.rfind("--config=", 0) == 0) {
            configPath = arg.substr(std::string("--config=").size());
            continue;
        }
        if (arg == "--module" && i + 1 < argc) {
            initialModulePath = argv[++i];
            continue;
        }
        if (arg.rfind("--module=", 0) == 0) {
            initialModulePath = arg.substr(std::string("--module=").size());
            continue;
        }
    }

    if (showHelp) {
        std::cout
            << "Usage: " << (argc > 0 ? argv[0] : "myAppModuleHost")
            << " [--config PATH] [--module <alias|path>]\n";
        printHelp();
        std::cout << "[module-host] aliases: cli, gui, echo\n";
        return 0;
    }

    sim::module::FrontendModuleHandle module{};
    std::string currentModuleSpecifier = initialModulePath;
    const std::string resolvedInitialModulePath = resolveModuleSpecifier(initialModulePath, searchRoots);
    std::string loadError;
    if (!loadModule(resolvedInitialModulePath, configPath, module, loadError)) {
        std::cerr << "[module-host] failed to load module '" << initialModulePath
                  << "': " << loadError << "\n";
        return 1;
    }

    std::cout << "[module-host] loaded: " << module.moduleName()
              << " (" << module.loadedPath() << ")\n";
    printHelp();

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
            printHelp();
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
        if ((tokens[0] == "quit") || (tokens[0] == "exit")) {
            break;
        }
        if (tokens[0] == "reload") {
            if (currentModuleSpecifier.empty()) {
                std::cout << "[module-host] no module specifier to reload\n";
                continue;
            }
            sim::module::FrontendModuleHandle replacement{};
            std::string switchError;
            const std::string resolvedPath = resolveModuleSpecifier(currentModuleSpecifier, searchRoots);
            if (!loadModule(resolvedPath, configPath, replacement, switchError)) {
                std::cout << "[module-host] reload failed: " << switchError << "\n";
                continue;
            }
            module = std::move(replacement);
            std::cout << "[module-host] reloaded: " << module.moduleName()
                      << " (" << module.loadedPath() << ")\n";
            continue;
        }
        if (tokens[0] == "switch") {
            if (tokens.size() < 2u) {
                std::cout << "[module-host] usage: switch <module_alias_or_path>\n";
                continue;
            }
            const std::string moduleSpecifier = tokens[1];
            const std::string resolvedPath = resolveModuleSpecifier(moduleSpecifier, searchRoots);
            sim::module::FrontendModuleHandle replacement{};
            std::string switchError;
            if (!loadModule(resolvedPath, configPath, replacement, switchError)) {
                std::cout << "[module-host] switch failed: " << switchError << "\n";
                continue;
            }
            module = std::move(replacement);
            currentModuleSpecifier = moduleSpecifier;
            std::cout << "[module-host] switched to: " << module.moduleName()
                      << " (" << module.loadedPath() << ")\n";
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
    } catch (const std::exception &ex) {
        std::cerr << "[module-host] fatal: " << ex.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "[module-host] fatal: unknown exception\n";
        return 1;
    }
}
