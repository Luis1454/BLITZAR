/*
 * @file runtime/command/execution/CmdExecutor.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime public interfaces for protocol, command, client, and FFI boundaries.
 */

#ifndef BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDEXECUTOR_HPP_
#define BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDEXECUTOR_HPP_
#include "command/core/CmdContext.hpp"
#include "command/core/CmdTypes.hpp"

namespace bltzr_cmd {
Result execute(const Request& request, ExecutionContext& context);
} // namespace bltzr_cmd
#endif // BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDEXECUTOR_HPP_
