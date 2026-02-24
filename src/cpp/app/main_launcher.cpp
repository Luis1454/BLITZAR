#include "sim/PlatformPaths.hpp"
#include "sim/PlatformProcess.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace {

enum class LaunchMode {
    Ui,
    Backend,
    Headless
};

struct LauncherOptions {
    LaunchMode mode = LaunchMode::Ui;
    std::string module = "cli";
    std::vector<std::string> passthroughArgs;
    bool showHelp = false;
};

void printUsage(const std::string_view programName)
{
    std::cout
        << "Usage: " << programName << " [--mode ui|backend|headless] [--module <name>] [args...]\n"
        << "  --mode      Select child process to run (default: ui).\n"
        << "  --module    Frontend module alias/path for ui mode (default: cli).\n"
        << "  --help      Show this help.\n"
        << "\nExamples:\n"
        << "  " << programName << " --mode ui --module qt -- --backend-host 127.0.0.1 --backend-port 4545\n"
        << "  " << programName << " --mode backend -- --config simulation.ini\n"
        << "  " << programName << " --mode headless -- --particle-count 10000 --target-steps 1000\n";
}

bool parseMode(const std::string &rawValue, LaunchMode &outMode)
{
    std::string value = rawValue;
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    if (value == "ui") {
        outMode = LaunchMode::Ui;
        return true;
    }
    if (value == "backend") {
        outMode = LaunchMode::Backend;
        return true;
    }
    if (value == "headless") {
        outMode = LaunchMode::Headless;
        return true;
    }
    return false;
}

bool parseLauncherOptions(
    const int argc,
    char **argv,
    LauncherOptions &outOptions,
    std::string &outError)
{
    bool sawDoubleDash = false;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i] == nullptr ? std::string() : std::string(argv[i]);

        if (sawDoubleDash) {
            outOptions.passthroughArgs.push_back(arg);
            continue;
        }

        if (arg == "--") {
            sawDoubleDash = true;
            continue;
        }
        if (arg == "--help" || arg == "-h") {
            outOptions.showHelp = true;
            continue;
        }
        if (arg == "--mode") {
            if (i + 1 >= argc) {
                outError = "--mode requires a value";
                return false;
            }
            const std::string modeValue = argv[++i] == nullptr ? std::string() : std::string(argv[i]);
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
            if (i + 1 >= argc) {
                outError = "--module requires a value";
                return false;
            }
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
    case LaunchMode::Ui:
        return "myAppModuleHost";
    case LaunchMode::Backend:
        return "myAppBackend";
    case LaunchMode::Headless:
        return "myAppHeadless";
    }
    return "myAppModuleHost";
}

bool containsModuleOverride(const std::vector<std::string> &args)
{
    return std::any_of(args.begin(), args.end(), [](const std::string &arg) {
        return arg == "--module" || arg.rfind("--module=", 0u) == 0u;
    });
}

std::string resolveExecutablePath(const std::string_view launcherPath, const std::string &executableName)
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

int runChildBlocking(const std::string &resolvedExecutable, const std::vector<std::string> &childArgs)
{
    sim::platform::ProcessHandle child;
    std::string launchError;
    if (!child.launch(resolvedExecutable, childArgs, false, launchError)) {
        std::cerr << "[launcher] failed to launch child process: "
                  << (launchError.empty() ? "unknown error" : launchError)
                  << '\n';
        return 1;
    }

    while (child.isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return 0;
}

} // namespace

int main(int argc, char **argv)
{
    LauncherOptions options{};
    std::string parseError;
    if (!parseLauncherOptions(argc, argv, options, parseError)) {
        std::cerr << "[launcher] " << parseError << '\n';
        printUsage(argc > 0 && argv != nullptr && argv[0] != nullptr ? argv[0] : "myApp");
        return 2;
    }

    if (options.showHelp) {
        printUsage(argc > 0 && argv != nullptr && argv[0] != nullptr ? argv[0] : "myApp");
        return 0;
    }

    std::vector<std::string> childArgs = options.passthroughArgs;
    if (options.mode == LaunchMode::Ui && !containsModuleOverride(childArgs)) {
        childArgs.push_back("--module");
        childArgs.push_back(options.module);
    }

    const std::string childBasename = targetBasename(options.mode);
    const std::string childExecutable = sim::platform::executableName(childBasename);
    const std::string resolvedExecutable =
        resolveExecutablePath(argc > 0 && argv != nullptr && argv[0] != nullptr ? argv[0] : "", childExecutable);
    return runChildBlocking(resolvedExecutable, childArgs);
}
