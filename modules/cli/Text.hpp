/*
 * @file modules/cli/Text.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Command-line client module for runtime control workflows.
 */

#ifndef BLITZAR_MODULES_CLI_TEXT_HPP_
#define BLITZAR_MODULES_CLI_TEXT_HPP_
#include <string>
#include <vector>

namespace bltzr_module_cli {
std::string trim(const std::string& input);
std::vector<std::string> splitTokens(const std::string& line);
} // namespace bltzr_module_cli
#endif // BLITZAR_MODULES_CLI_TEXT_HPP_
