#include "client/LocalServerFactory.hpp"

#include "server/SimulationServer.hpp"

#include <memory>
#include <string>

namespace grav_client {

std::unique_ptr<grav_client::ILocalServer> createLocalServer(const std::string &configPath)
{
    return std::make_unique<SimulationServer>(configPath);
}

} // namespace grav_client

