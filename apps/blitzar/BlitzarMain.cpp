#include "BlitzarPostProcess.hpp"
#include "BlitzarRun.hpp"
#include "BlitzarSummary.hpp"

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string_view>

namespace {

enum class BlitzarCommandKind : std::uint8_t { Invalid, Config, PostProcess, Smoke };

struct BlitzarCommandLine final {
    BlitzarCommandKind kind{BlitzarCommandKind::Invalid};
    blitzar_cli::BlitzarOutputFormat format{blitzar_cli::BlitzarOutputFormat::Human};
    std::string_view path;
};

[[nodiscard]] bool ParseFormat(
    std::string_view value, blitzar_cli::BlitzarOutputFormat& format) noexcept
{
    if (value == "human") {
        format = blitzar_cli::BlitzarOutputFormat::Human;

        return true;
    }

    if (value == "json") {
        format = blitzar_cli::BlitzarOutputFormat::Json;

        return true;
    }

    return false;
}

[[nodiscard]] bool ParseCommand(std::string_view value, BlitzarCommandKind& command) noexcept
{
    if (value == "--config") {
        command = BlitzarCommandKind::Config;

        return true;
    }

    if (value == "--post-process") {
        command = BlitzarCommandKind::PostProcess;

        return true;
    }

    return false;
}

[[nodiscard]] BlitzarCommandLine ParseCommandLine(int argc, char** argv) noexcept
{
    BlitzarCommandLine result;

    if (argc == 1) {
        result.kind = BlitzarCommandKind::Smoke;

        return result;
    }

    if (argc >= 3 && std::string_view(argv[1]) == "--format") {
        const bool format_valid = ParseFormat(argv[2], result.format);

        if (format_valid && argc == 5 && ParseCommand(argv[3], result.kind)) {
            result.path = argv[4];
        }
        else if (!format_valid) {
            result.kind = BlitzarCommandKind::Invalid;
        }

        return result;
    }

    if (argc >= 3 && ParseCommand(argv[1], result.kind)) {
        result.path = argv[2];

        if (argc == 5 && std::string_view(argv[3]) == "--format") {
            if (!ParseFormat(argv[4], result.format)) {
                result.kind = BlitzarCommandKind::Invalid;
            }
        }
        else if (argc != 3) {
            result.kind = BlitzarCommandKind::Invalid;
        }
    }

    return result;
}

} // namespace

int main(int argc, char** argv)
{
    const BlitzarCommandLine command = ParseCommandLine(argc, argv);

    if (command.kind == BlitzarCommandKind::Smoke) {
        return blitzar_cli::RunSmoke();
    }

    if (command.kind == BlitzarCommandKind::Config) {
        return blitzar_cli::RunConfig(
            std::filesystem::path(command.path), {std::cout, std::cerr, command.format});
    }

    if (command.kind == BlitzarCommandKind::PostProcess) {
        return blitzar_cli::RunPostProcess(
            std::filesystem::path(command.path), {std::cout, std::cerr, command.format});
    }

    const blitzar_cli::BlitzarFailure failure{
        BLITZAR_STATUS_INVALID_ARGUMENT, "usage", blitzar_cli::BlitzarExitCode::Usage};

    (void)blitzar_cli::WriteFailure(std::cerr, failure, command.format);

    return static_cast<int>(blitzar_cli::BlitzarExitCode::Usage);
}
