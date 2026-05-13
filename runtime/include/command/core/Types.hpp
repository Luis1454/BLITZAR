/*
 * @file runtime/include/command/core/Types.hpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Runtime public interfaces for protocol, command, client, and FFI boundaries.
 */

#ifndef BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDTYPES_HPP_
#define BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDTYPES_HPP_
#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace bltzr_cmd {
enum class Id {
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
enum class ArgumentKind {
    String,
    Uint,
    Float,
    Path,
    Host,
    Port,
    Enum
};
enum class ExecutionMode {
    Interactive,
    Batch
};
typedef std::variant<std::string, std::uint64_t, double> ArgumentValue;

struct ArgumentSpec final {
    std::string name;
    ArgumentKind kind = ArgumentKind::String;
    bool optional = false;
    std::vector<std::string> suggestions;
};

struct Spec final {
    Id id = Id::Help;
    std::string name;
    std::string help;
    bool deterministic = true;
    std::vector<ArgumentSpec> arguments;
};

struct Request final {
    Id id = Id::Help;
    std::string name;
    std::size_t lineNumber = 0u;
    std::vector<ArgumentValue> arguments;
};

struct ParseResult final {
    bool ok = false;
    std::vector<Request> requests;
    std::string error;
};

struct Result final {
    bool ok = false;
    std::string message;
};
} // namespace bltzr_cmd
#endif // BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDTYPES_HPP_
