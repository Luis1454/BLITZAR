/*
 * @file runtime/include/command/CommandCatalog.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime public interfaces for protocol, command, client, and FFI boundaries.
 */

#ifndef BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDCATALOG_HPP_
#define BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDCATALOG_HPP_
#include "command/CommandTypes.hpp"
#include <string_view>
#include <vector>

namespace bltzr_cmd {
class CommandCatalog final {
public:
    static const CommandSpec* findByName(std::string_view name);
    static const CommandSpec* findById(CommandId id);
    static const std::vector<CommandSpec>& all();
    static std::string renderHelp();
};
} // namespace bltzr_cmd
#endif // BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDCATALOG_HPP_
