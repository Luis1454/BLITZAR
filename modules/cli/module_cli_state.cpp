/*
 * @file modules/cli/module_cli_state.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Command-line client module for runtime control workflows.
 */

#include "modules/cli/module_cli_state.hpp"
#include "config/SimulationConfig.hpp"

namespace grav_module_cli {
ModuleState::ModuleState() : transport(150), session()
{
    session.config = SimulationConfig::defaults();
}
} // namespace grav_module_cli
