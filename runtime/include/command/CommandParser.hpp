// File: runtime/include/command/CommandParser.hpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#ifndef GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDPARSER_HPP_
#define GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDPARSER_HPP_
#include "command/CommandTypes.hpp"
#include <cstddef>
#include <string>
namespace grav_cmd {
/// Description: Defines the CommandParser data or behavior contract.
class CommandParser final {
public:
    /// Description: Executes the parseScript operation.
    static CommandParseResult parseScript(const std::string& scriptText);
    /// Description: Executes the parseLine operation.
    static CommandParseResult parseLine(const std::string& line, std::size_t lineNumber);
};
} // namespace grav_cmd
#endif // GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDPARSER_HPP_
