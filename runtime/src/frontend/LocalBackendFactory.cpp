#include "frontend/LocalBackendFactory.hpp"

#include "backend/SimulationBackend.hpp"

#include <memory>
#include <string>

namespace grav_frontend {

std::unique_ptr<grav_frontend::ILocalBackend> createLocalBackend(const std::string &configPath)
{
    return std::make_unique<SimulationBackend>(configPath);
}

} // namespace grav_frontend

