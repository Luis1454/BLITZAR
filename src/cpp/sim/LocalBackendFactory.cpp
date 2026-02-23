#include "sim/LocalBackendFactory.hpp"

#include "sim/SimulationBackend.hpp"

#include <memory>
#include <string>

namespace sim {

std::unique_ptr<ILocalBackend> createLocalBackend(const std::string &configPath)
{
    return std::make_unique<SimulationBackend>(configPath);
}

} // namespace sim
