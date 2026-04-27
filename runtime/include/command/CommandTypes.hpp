// File: runtime/include/command/CommandTypes.hpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#ifndef GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDTYPES_HPP_
#define GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDTYPES_HPP_
#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>
namespace grav_cmd {
enum class CommandId {
    Help,
    LoadConfig,
    Connect,
    Reconnect,
    Status,
    Pause,
    Resume,
    Toggle,
    Step,
    Reset,
    Recover,
    Shutdown,
    SetDt,
    SetSolver,
    SetIntegrator,
    SetProfile,
    SetParticleCount,
    ExportSnapshot,
    SaveCheckpoint,
    LoadCheckpoint,
    RunSteps,
    RunUntil
};
enum class CommandArgumentKind { String, Uint, Float, Path, Host, Port, Enum };
enum class CommandExecutionMode { Interactive, Batch };
typedef std::variant<std::string, std::uint64_t, double> CommandArgumentValue;
struct CommandArgumentSpec final {
    std::string name;
    CommandArgumentKind kind = CommandArgumentKind::String;
    bool optional = false;
    std::vector<std::string> suggestions;
};
struct CommandSpec final {
    CommandId id = CommandId::Help;
    std::string name;
    std::string help;
    bool deterministic = true;
    std::vector<CommandArgumentSpec> arguments;
};
struct CommandRequest final {
    CommandId id = CommandId::Help;
    std::string name;
    std::size_t lineNumber = 0u;
    std::vector<CommandArgumentValue> arguments;
};
struct CommandParseResult final {
    bool ok = false;
    std::vector<CommandRequest> requests;
    std::string error;
};
struct CommandResult final {
    bool ok = false;
    std::string message;
};
} // namespace grav_cmd
#endif // GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDTYPES_HPP_
