# @file engine/config/directive/Module.cmake
# @brief Directive parser and writer sources.

set(BLITZAR_CONFIG_DIRECTIVE_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/config/directive")
set(BLITZAR_CONFIG_DIRECTIVE_SOURCE_DIR "${BLITZAR_ROOT_DIR}/engine/config/directive")
set(BLITZAR_CONFIG_DIRECTIVE_SOURCES
    "${BLITZAR_CONFIG_DIRECTIVE_SOURCE_DIR}/CfgConfig.cpp"
    "${BLITZAR_CONFIG_DIRECTIVE_SOURCE_DIR}/CfgOptions.cpp"
    "${BLITZAR_CONFIG_DIRECTIVE_SOURCE_DIR}/CfgParser.cpp"
    "${BLITZAR_CONFIG_DIRECTIVE_SOURCE_DIR}/CfgScene.cpp"
    "${BLITZAR_CONFIG_DIRECTIVE_SOURCE_DIR}/CfgLegacyScene.cpp"
    "${BLITZAR_CONFIG_DIRECTIVE_SOURCE_DIR}/CfgWrite.cpp"
    "${BLITZAR_CONFIG_DIRECTIVE_SOURCE_DIR}/CfgCore.cpp"
    "${BLITZAR_CONFIG_DIRECTIVE_SOURCE_DIR}/CfgInitialState.cpp"
    "${BLITZAR_CONFIG_DIRECTIVE_SOURCE_DIR}/CfgSceneWriter.cpp"
    "${BLITZAR_CONFIG_DIRECTIVE_SOURCE_DIR}/CfgStreamWriter.cpp"
    "${BLITZAR_CONFIG_DIRECTIVE_SOURCE_DIR}/CfgValueFormatter.cpp"
)
