/*
 * @file runtime/command/execution/CmdBatchRunner.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime public interfaces for protocol, command, client, and FFI boundaries.
 */

#ifndef BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDBATCHRUNNER_HPP_
#define BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDBATCHRUNNER_HPP_
#include "command/core/CmdContext.hpp"
#include "command/core/CmdTypes.hpp"
#include <string>

namespace bltzr_cmd {
Result runScriptFile(const std::string& path, ExecutionContext& context);
} // namespace bltzr_cmd
#endif // BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDBATCHRUNNER_HPP_
