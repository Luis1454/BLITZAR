/*
 * @file engine/config/directive/CfgConfig.cpp
 * @brief Public directive application facade.
 */

#include "config/directive/CfgConfig.hpp"

#include "CfgDirectiveInternals.hpp"

#include "config/core/CfgConfig.hpp"

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
