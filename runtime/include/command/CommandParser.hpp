/*
 * @file runtime/include/command/CommandParser.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime public interfaces for protocol, command, client, and FFI boundaries.
 */

#ifndef BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDPARSER_HPP_
#define BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDPARSER_HPP_
#include "command/CommandTypes.hpp"
#include <cstddef>
#include <string>

namespace bltzr_cmd {
class CommandParser final {
public:
    static CommandParseResult parseScript(const std::string& scriptText);
    static CommandParseResult parseLine(const std::string& line, std::size_t lineNumber);
};
} // namespace bltzr_cmd
#endif // BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDPARSER_HPP_
