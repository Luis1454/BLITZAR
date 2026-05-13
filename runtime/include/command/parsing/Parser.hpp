/*
 * @file runtime/include/command/parsing/Parser.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime public interfaces for protocol, command, client, and FFI boundaries.
 */

#ifndef BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDPARSER_HPP_
#define BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDPARSER_HPP_
#include "command/core/Types.hpp"
#include <cstddef>
#include <string>

namespace bltzr_cmd {
ParseResult parseScript(const std::string& scriptText);
ParseResult parseLine(const std::string& line, std::size_t lineNumber);
} // namespace bltzr_cmd
#endif // BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDPARSER_HPP_
