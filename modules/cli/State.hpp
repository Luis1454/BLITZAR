/*
 * @file modules/cli/State.hpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Command-line client module for runtime control workflows.
 */

#ifndef BLITZAR_MODULES_CLI_STATE_HPP_
#define BLITZAR_MODULES_CLI_STATE_HPP_
#include "command/core/Context.hpp"
#include "command/transport/Transport.hpp"

namespace bltzr_module_cli {
struct State {
    State();
    bltzr_cmd::ServerTransport transport;
    bltzr_cmd::SessionState session;
};
} // namespace bltzr_module_cli
#endif // BLITZAR_MODULES_CLI_STATE_HPP_
