#include "BlitzarRun.hpp"
#include "BlitzarSummary.hpp"

#include <filesystem>
#include <iostream>
#include <string_view>

int main(int argc, char** argv)
{
    if (argc == 1) {
        return blitzar_cli::RunSmoke();
    }

    if (argc != 3 || std::string_view(argv[1]) != "--config") {
        const blitzar_cli::BlitzarFailure failure{
            BLITZAR_STATUS_INVALID_ARGUMENT, "usage", blitzar_cli::BlitzarExitCode::Usage};

        (void)blitzar_cli::WriteFailure(std::cerr, failure);

        return static_cast<int>(blitzar_cli::BlitzarExitCode::Usage);
    }

    return blitzar_cli::RunConfig(std::filesystem::path(argv[2]));
}
