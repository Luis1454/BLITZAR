#ifndef GRAVITY_RUNTIME_INCLUDE_CLIENT_LOCALSERVERFACTORY_HPP_
#define GRAVITY_RUNTIME_INCLUDE_CLIENT_LOCALSERVERFACTORY_HPP_

#include "client/ILocalServer.hpp"

#include <memory>
#include <string>

namespace grav_client {

std::unique_ptr<grav_client::ILocalServer> createLocalServer(const std::string &configPath);

} // namespace grav_client



#endif // GRAVITY_RUNTIME_INCLUDE_CLIENT_LOCALSERVERFACTORY_HPP_
