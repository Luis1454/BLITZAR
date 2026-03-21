#ifndef GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDCONTEXT_HPP_
#define GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDCONTEXT_HPP_

#include "command/CommandTransport.hpp"
#include "command/CommandTypes.hpp"
#include "config/SimulationConfig.hpp"

#include <cstdint>
#include <iosfwd>
#include <string>

namespace grav_cmd {

struct CommandSessionState final {
    SimulationConfig config{};
    std::string configPath = "simulation.ini";
    std::string host = "127.0.0.1";
    std::uint16_t port = 4545u;
};

struct CommandExecutionContext final {
    CommandTransport &transport;
    CommandSessionState &session;
    CommandExecutionMode mode = CommandExecutionMode::Interactive;
    std::ostream &output;
};

} // namespace grav_cmd

#endif // GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDCONTEXT_HPP_
