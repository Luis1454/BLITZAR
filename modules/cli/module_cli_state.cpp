// File: modules/cli/module_cli_state.cpp
// Purpose: Client module implementation for BLITZAR extension workflows.

#include "modules/cli/module_cli_state.hpp"
#include "config/SimulationConfig.hpp"
namespace grav_module_cli {
/// Description: Executes the ModuleState operation.
ModuleState::ModuleState() : transport(150), session()
{
    session.config = SimulationConfig::defaults();
}
} // namespace grav_module_cli
