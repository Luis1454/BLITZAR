#ifndef BLITZAR_APPS_BLITZAR_BLITZAR_RUN_HPP
#define BLITZAR_APPS_BLITZAR_BLITZAR_RUN_HPP

#include <filesystem>

namespace blitzar_cli {

int RunSmoke();
int RunConfig(const std::filesystem::path& path);

} // namespace blitzar_cli

#endif
