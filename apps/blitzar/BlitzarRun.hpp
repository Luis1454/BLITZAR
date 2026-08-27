#ifndef BLITZAR_APPS_BLITZAR_BLITZAR_RUN_HPP
#define BLITZAR_APPS_BLITZAR_BLITZAR_RUN_HPP

#include "BlitzarStreams.hpp"

#include <filesystem>

namespace blitzar_cli {

int RunSmoke();
int RunConfig(const std::filesystem::path& path);
int RunConfig(const std::filesystem::path& path, BlitzarStreams streams);

} // namespace blitzar_cli

#endif
