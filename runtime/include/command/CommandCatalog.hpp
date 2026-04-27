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
    /// Description: Executes the findByName operation.
    static const CommandSpec* findByName(std::string_view name);
    /// Description: Executes the findById operation.
    static const CommandSpec* findById(CommandId id);
    /// Description: Executes the all operation.
    static const std::vector<CommandSpec>& all();
    /// Description: Executes the renderHelp operation.
    static std::string renderHelp();
};
} // namespace grav_cmd
#endif // GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDCATALOG_HPP_
