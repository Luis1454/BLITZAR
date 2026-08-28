#ifndef BLITZAR_APPS_BLITZAR_BLITZAR_RUN_HPP
#define BLITZAR_APPS_BLITZAR_BLITZAR_RUN_HPP

#include "BlitzarRunTiming.hpp"
#include "BlitzarStreams.hpp"

#include <filesystem>

namespace blitzar_cli {

int RunSmoke();
int RunConfig(const std::filesystem::path& path);
int RunConfig(const std::filesystem::path& path, BlitzarStreams streams);
int RunConfig(const std::filesystem::path& path, BlitzarStreams streams, BlitzarRunTiming& timing);

} // namespace blitzar_cli

#endif
