/*
 * @file modules/cli/state/CliState.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Command-line client module for runtime control workflows.
 */

#include "modules/cli/state/CliState.hpp"
#include "config/core/configuration/CfgConfig.hpp"

namespace bltzr_module_cli {

State::State() : transport(150), session()
{
    session.config = SimulationConfig::defaults();
}

} // namespace bltzr_module_cli
