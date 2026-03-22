#ifndef GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDCATALOG_HPP_
#define GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDCATALOG_HPP_

#include "command/CommandTypes.hpp"

#include <string_view>
#include <vector>

namespace grav_cmd {

class CommandCatalog final {
public:
    static const CommandSpec *findByName(std::string_view name);
    static const CommandSpec *findById(CommandId id);
    static const std::vector<CommandSpec> &all();
    static std::string renderHelp();
};

} // namespace grav_cmd

#endif // GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDCATALOG_HPP_
