/*
 * @file runtime/include/command/core/Context.hpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Runtime public interfaces for protocol, command, client, and FFI boundaries.
 */

#ifndef BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDCONTEXT_HPP_
#define BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDCONTEXT_HPP_
#include "Constants.hpp"
#include "command/transport/Transport.hpp"
#include "command/core/Types.hpp"
#include "config/core/Config.hpp"
#include <cstdint>
#include <iosfwd>
#include <string>

namespace bltzr_cmd {
struct SessionState final {
    SimulationConfig config{};
    std::string configPath = "simulation.ini";
    std::string host = kDefaultLoopbackHost;
    std::uint16_t port = kDefaultServerPort;
};

struct ExecutionContext final {
    Transport& transport;
    SessionState& session;
    ExecutionMode mode = ExecutionMode::Interactive;
    std::ostream& output;
};
} // namespace bltzr_cmd
#endif // BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDCONTEXT_HPP_
