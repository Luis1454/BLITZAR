#ifndef GRAVITY_SIM_LOCALBACKENDFACTORY_HPP
#define GRAVITY_SIM_LOCALBACKENDFACTORY_HPP

#include "frontend/ILocalBackend.hpp"

#include <memory>
#include <string>

namespace grav_frontend {

std::unique_ptr<grav_frontend::ILocalBackend> createLocalBackend(const std::string &configPath);

} // namespace grav_frontend

#endif // GRAVITY_SIM_LOCALBACKENDFACTORY_HPP

