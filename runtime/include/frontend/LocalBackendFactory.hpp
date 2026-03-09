#pragma once

#include "frontend/ILocalBackend.hpp"

#include <memory>
#include <string>

namespace grav_frontend {

std::unique_ptr<grav_frontend::ILocalBackend> createLocalBackend(const std::string &configPath);

} // namespace grav_frontend


