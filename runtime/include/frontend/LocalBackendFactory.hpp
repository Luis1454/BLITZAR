#ifndef GRAVITY_RUNTIME_INCLUDE_FRONTEND_LOCALBACKENDFACTORY_HPP_
#define GRAVITY_RUNTIME_INCLUDE_FRONTEND_LOCALBACKENDFACTORY_HPP_

#include "frontend/ILocalBackend.hpp"

#include <memory>
#include <string>

namespace grav_frontend {

std::unique_ptr<grav_frontend::ILocalBackend> createLocalBackend(const std::string &configPath);

} // namespace grav_frontend



#endif // GRAVITY_RUNTIME_INCLUDE_FRONTEND_LOCALBACKENDFACTORY_HPP_
