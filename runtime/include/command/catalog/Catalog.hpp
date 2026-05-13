/*
 * @file runtime/include/command/catalog/Catalog.hpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Runtime public interfaces for protocol, command, client, and FFI boundaries.
 */

#ifndef BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDCATALOG_HPP_
#define BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDCATALOG_HPP_
#include "command/core/Types.hpp"
#include <string_view>
#include <vector>

namespace bltzr_cmd {
class Catalog final {
public:
    static const Spec* findByName(std::string_view name);
    static const Spec* findById(Id id);
    static const std::vector<Spec>& all();
    static std::string renderHelp();
};
} // namespace bltzr_cmd
#endif // BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDCATALOG_HPP_
