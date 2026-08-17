/*
 * @file runtime/command/core/CmdContext.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime public interfaces for protocol, command, client, and FFI boundaries.
 */

#ifndef BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDCONTEXT_HPP_
#define BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDCONTEXT_HPP_
#include "FndConstants.hpp"
#include "command/transport/CmdTransport.hpp"
#include "command/core/CmdTypes.hpp"
#include "config/core/CfgConfig.hpp"
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
