#ifndef GRAVITY_SIM_LOCALBACKENDFACTORY_HPP
#define GRAVITY_SIM_LOCALBACKENDFACTORY_HPP

#include "sim/ILocalBackend.hpp"

#include <memory>
#include <string>

namespace sim {

std::unique_ptr<ILocalBackend> createLocalBackend(const std::string &configPath);

} // namespace sim

#endif // GRAVITY_SIM_LOCALBACKENDFACTORY_HPP
