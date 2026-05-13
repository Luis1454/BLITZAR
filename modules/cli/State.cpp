/*
 * @file modules/cli/State.cpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Command-line client module for runtime control workflows.
 */

#include "modules/cli/State.hpp"
#include "config/core/Config.hpp"

namespace bltzr_module_cli {

State::State() : transport(150), session()
{
    session.config = SimulationConfig::defaults();
}

} // namespace bltzr_module_cli
