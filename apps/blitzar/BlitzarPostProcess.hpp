#ifndef BLITZAR_APPS_BLITZAR_BLITZAR_POST_PROCESS_HPP
#define BLITZAR_APPS_BLITZAR_BLITZAR_POST_PROCESS_HPP

#include "BlitzarStreams.hpp"

#include <filesystem>

namespace blitzar_cli {

int RunPostProcess(const std::filesystem::path& path);
int RunPostProcess(const std::filesystem::path& path, BlitzarStreams streams);

} // namespace blitzar_cli

#endif
