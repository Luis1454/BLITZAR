// File: runtime/include/command/CommandCatalog.hpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#ifndef GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDCATALOG_HPP_
#define GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDCATALOG_HPP_
#include "command/CommandTypes.hpp"
#include <string_view>
#include <vector>

namespace grav_cmd {
/// Description: Defines the CommandCatalog data or behavior contract.
class CommandCatalog final {
public:
    /// Description: Describes the find by name operation contract.
    static const CommandSpec* findByName(std::string_view name);
    /// Description: Describes the find by id operation contract.
    static const CommandSpec* findById(CommandId id);
    /// Description: Describes the all operation contract.
    static const std::vector<CommandSpec>& all();
    /// Description: Describes the render help operation contract.
    static std::string renderHelp();
};
} // namespace grav_cmd
#endif // GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDCATALOG_HPP_
