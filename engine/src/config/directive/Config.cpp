/*
 * @file engine/src/config/directive/Config.cpp
 * @brief Public directive application facade.
 */

#include "config/directive/Config.hpp"

#include "DirectiveInternals.hpp"

#include "config/core/Config.hpp"

namespace bltzr_config {
bool SimulationConfigDirective::applyLine(const std::string& line, SimulationConfig& config,
                                          std::ostream& warnings)
{
    std::string directive;
    DirectiveArguments args;
    if (!parseDirective(line, directive, args)) {
        return false;
    }
    if (directive.empty()) {
        warnings << "[config] invalid directive ignored: " << line << "\n";
        return true;
    }
    return applyDirectiveArgs(directive, args, config, warnings);
}
} // namespace bltzr_config
