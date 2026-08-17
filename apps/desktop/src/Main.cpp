/*
 * @file apps/desktop/src/Main.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Console-free desktop entry point for the BLITZAR Qt client.
 */

#include "Main.hpp"
#include "PltPaths.hpp"
#include "PltProcess.hpp"
#include <filesystem>
#include <string>
#include <vector>

namespace bltzr_desktop {
std::string siblingPath(const std::string& executablePath, const std::string& name)
{
    std::error_code error;
    const std::filesystem::path executable(executablePath);
    const std::filesystem::path absolute = std::filesystem::absolute(executable, error);
    if (error || !absolute.has_parent_path()) {
        return name;
    }
    return (absolute.parent_path() / name).string();
}

std::string defaultConfigPath(const std::string& executablePath)
{
    const std::string candidate = siblingPath(executablePath, "simulation.ini");
    std::error_code error;
    if (std::filesystem::exists(candidate, error) && !error) {
        return candidate;
    }
    return "simulation.ini";
}

bool parseConfigPath(int argc, char** argv, std::string& outConfig, std::string& outError)
{
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index] == nullptr ? std::string() : argv[index];
        if (argument == "--config") {
            if (index + 1 >= argc || argv[index + 1] == nullptr) {
                outError = "--config requires a file path";
                return false;
            }
            outConfig = argv[++index];
            continue;
        }
        if (argument.rfind("--config=", 0u) == 0u) {
            outConfig = argument.substr(std::string("--config=").size());
            continue;
        }
        if (argument == "--help" || argument == "-h") {
            outError = "BLITZAR GUI starts the Qt desktop client. Optional argument: --config PATH";
            return false;
        }
        outError = "unknown argument: " + argument;
        return false;
    }
    return true;
}

int run(int argc, char** argv)
{
    const std::string executablePath =
        argc > 0 && argv != nullptr && argv[0] != nullptr ? argv[0] : "blitzar-gui";
    std::string configPath = defaultConfigPath(executablePath);
    std::string parseError;
    if (!parseConfigPath(argc, argv, configPath, parseError)) {
        showError(parseError);
        return 2;
    }

    const std::string clientExecutable = siblingPath(
        executablePath, bltzr_platform::executableName("blitzar-client"));
    const std::vector<std::string> clientArguments = {
        "--config", configPath, "--module", "qt", "--wait-for-module"};
    std::string launchError;
    const int exitCode =
        bltzr_platform::runProcessBlocking(clientExecutable, clientArguments, false, launchError);
    if (!launchError.empty()) {
        showError("Unable to start the BLITZAR client host: " + launchError);
        return 1;
    }
    if (exitCode != 0) {
        showError("The BLITZAR client stopped with exit code " + std::to_string(exitCode) + ".");
    }
    return exitCode;
}
} // namespace bltzr_desktop
