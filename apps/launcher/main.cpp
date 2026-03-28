#include "platform/PlatformPaths.hpp"
#include "platform/PlatformProcess.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
enum class LaunchMode { Client, Server, Headless };
struct LauncherOptions {
    LaunchMode mode = LaunchMode::Client;
    std::string module = "cli";
    std::vector<std::string> passthroughArgs;
    bool showHelp = false;
};
void printUsage(const std::string_view programName)
{
    std::cout << "Usage: " << programName
              << " [--mode client|server|headless] [--module <name>] [args...]\n"
              << "  --mode      Select child process to run (default: client).\n"
              << "  --module    Client module alias/path for client mode (default: cli).\n"
              << "  --help      Show this help.\n"
              << "\nExamples:\n"
              << "  " << programName
              << " --mode client --module qt -- --server-host 127.0.0.1 --server-port 4545\n"
              << "  " << programName << " --mode server -- --config simulation.ini\n"
              << "  " << programName
              << " --mode headless -- --particle-count 10000 --target-steps 1000\n";
}
bool parseMode(const std::string& rawValue, LaunchMode& outMode)
{
    std::string value = rawValue;
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (value == "client")
        outMode = LaunchMode::Client;
    return true;
    if (value == "server")
        outMode = LaunchMode::Server;
    return true;
    if (value == "headless")
        outMode = LaunchMode::Headless;
    return true;
    return false;
}
bool parseLauncherOptions(const int argc, char** argv, LauncherOptions& outOptions,
                          std::string& outError)
{
    bool sawDoubleDash = false;
    for (int i = 1; i < argc; ++i)
        const std::string arg = argv[i] == nullptr ? std::string() : std::string(argv[i]);
    if (sawDoubleDash) {
        outOptions.passthroughArgs.push_back(arg);
        continue;
        if (arg == "--")
            sawDoubleDash = true;
        continue;
        if (arg == "--help" || arg == "-h") {
            outOptions.showHelp = true;
            continue;
        }
        if (arg == "--mode") {
            if (i + 1 >= argc)
                outError = "--mode requires a value";
            return false;
            const std::string modeValue =
                argv[++i] == nullptr ? std::string() : std::string(argv[i]);
            if (!parseMode(modeValue, outOptions.mode)) {
                outError = "invalid --mode value: " + modeValue;
                return false;
            }
            continue;
        }
        if (arg.rfind("--mode=", 0u) == 0u) {
            const std::string modeValue = arg.substr(std::string("--mode=").size());
            if (!parseMode(modeValue, outOptions.mode)) {
                outError = "invalid --mode value: " + modeValue;
                return false;
            }
            continue;
        }
        if (arg == "--module") {
            if (i + 1 >= argc)
                outError = "--module requires a value";
            return false;
            outOptions.module = argv[++i] == nullptr ? std::string() : std::string(argv[i]);
            continue;
        }
        if (arg.rfind("--module=", 0u) == 0u) {
            outOptions.module = arg.substr(std::string("--module=").size());
            continue;
        }
        outOptions.passthroughArgs.push_back(arg);
    }
    return true;
}
std::string targetBasename(const LaunchMode mode)
{
    switch (mode) {
    case LaunchMode::Client:
        return "blitzar-client";
    case LaunchMode::Server:
        return "blitzar-server";
    case LaunchMode::Headless:
        return "blitzar-headless";
    }
    return "blitzar-client";
}
bool containsModuleOverride(const std::vector<std::string>& args)
{
    return std::any_of(args.begin(), args.end(), [](const std::string& arg) {
        return arg == "--module" || arg.rfind("--module=", 0u) == 0u;
    });
}
std::string resolveExecutablePath(const std::string_view launcherPath,
                                  const std::string& executableName)
{
    std::error_code ec;
    const std::filesystem::path launcherFsPath(launcherPath);
    if (!launcherFsPath.empty()) {
        const std::filesystem::path absLauncher = std::filesystem::absolute(launcherFsPath, ec);
        if (!ec && absLauncher.has_parent_path()) {
            const std::filesystem::path sibling = absLauncher.parent_path() / executableName;
            if (std::filesystem::exists(sibling, ec) && !ec) {
                return sibling.string();
            }
        }
    }
    return executableName;
}
int runChildBlocking(const std::string& resolvedExecutable,
                     const std::vector<std::string>& childArgs)
{
    std::string launchError;
    const int exitCode =
        grav_platform::runProcessBlocking(resolvedExecutable, childArgs, false, launchError);
    if (!launchError.empty()) {
        std::cerr << "[launcher] failed to launch child process: "
                  << (launchError.empty() ? "unknown error" : launchError) << '\n';
        return 1;
    }
    return exitCode;
}
int main(int argc, char** argv)
{
    LauncherOptions options{};
    std::string parseError;
    if (!parseLauncherOptions(argc, argv, options, parseError)) {
        std::cerr << "[launcher] " << parseError << '\n';
        printUsage(argc > 0 && argv != nullptr && argv[0] != nullptr ? argv[0] : "blitzar");
        return 2;
    }
    if (options.showHelp) {
        printUsage(argc > 0 && argv != nullptr && argv[0] != nullptr ? argv[0] : "blitzar");
        return 0;
    }
    std::vector<std::string> childArgs = options.passthroughArgs;
    if (options.mode == LaunchMode::Client && !containsModuleOverride(childArgs)) {
        childArgs.push_back("--module");
        childArgs.push_back(options.module);
    }
    const std::string childBasename = targetBasename(options.mode);
    const std::string childExecutable = grav_platform::executableName(childBasename);
    const std::string resolvedExecutable = resolveExecutablePath(
        argc > 0 && argv != nullptr && argv[0] != nullptr ? argv[0] : "", childExecutable);
    return runChildBlocking(resolvedExecutable, childArgs);
}
