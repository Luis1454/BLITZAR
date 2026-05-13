/*
 * @file modules/cli/ServerOps.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Command-line client module for runtime control workflows.
 */

#ifndef BLITZAR_MODULES_CLI_SERVEROPS_HPP_
#define BLITZAR_MODULES_CLI_SERVEROPS_HPP_
#include "client/diagnostics/ErrorBuffer.hpp"
#include "modules/cli/State.hpp"
#include <string>
#include <vector>

namespace bltzr_module_cli {
class ServerOps final {
public:
    static bool commandStatus(State& state, const bltzr_client::ErrorBufferView& errorBuffer);

    static bool commandStep(State& state, const std::vector<std::string>& tokens,
                            const bltzr_client::ErrorBufferView& errorBuffer);

    static bool connect(State& state, const std::vector<std::string>& tokens,
                        const bltzr_client::ErrorBufferView& errorBuffer);

    static bool reconnect(State& state, const bltzr_client::ErrorBufferView& errorBuffer);

    static bool sendSimpleCommand(State& state, const std::string& cmd,
                                  const bltzr_client::ErrorBufferView& errorBuffer);
};
} // namespace bltzr_module_cli
#endif // BLITZAR_MODULES_CLI_SERVEROPS_HPP_
