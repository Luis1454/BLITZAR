/*
 * @file engine/src/config/directive/DirectiveInternals.hpp
 * @brief Private contracts shared by the directive parser components.
 */

#ifndef BLITZAR_ENGINE_SRC_CONFIG_DIRECTIVE_DIRECTIVEINTERNALS_HPP_
#define BLITZAR_ENGINE_SRC_CONFIG_DIRECTIVE_DIRECTIVEINTERNALS_HPP_

#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

struct SceneConfig;
struct SceneObjectConfig;
struct SimulationConfig;

namespace bltzr_config {
typedef std::pair<std::string, std::string> DirectiveArgument;
typedef std::vector<DirectiveArgument> DirectiveArguments;

bool parseDirective(std::string_view raw, std::string& name, DirectiveArguments& args);

bool parseSceneFloat(const std::string& raw, float& target);
bool parseSceneUint(const std::string& raw, std::uint32_t& target);
bool parseSceneBool(const std::string& raw, bool& target);

bool applySceneObjectArg(const DirectiveArgument& arg, SceneObjectConfig& object);
void applyLegacySceneProperty(const DirectiveArguments& args, SceneConfig& scene,
                              std::ostream& warnings);

bool applyDirectiveArgs(std::string_view directive, const DirectiveArguments& args,
                        SimulationConfig& config, std::ostream& warnings);
} // namespace bltzr_config

#endif // BLITZAR_ENGINE_SRC_CONFIG_DIRECTIVE_DIRECTIVEINTERNALS_HPP_
