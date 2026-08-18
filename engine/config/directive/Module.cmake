# @file engine/config/directive/Module.cmake
# @brief Directive parser and writer sources.

set(BLITZAR_CONFIG_DIRECTIVE_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/config/directive")
set(BLITZAR_CONFIG_DIRECTIVE_SOURCE_DIR "${BLITZAR_ROOT_DIR}/engine/config/directive")
set(BLITZAR_CONFIG_DIRECTIVE_SOURCES
    "${BLITZAR_ROOT_DIR}/engine/config/directive/parsing/CfgConfig.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/parsing/CfgOptions.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/parsing/CfgParser.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/scene/CfgScene.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/scene/CfgLegacyScene.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/write/CfgWrite.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/parsing/CfgCore.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/parsing/CfgInitialState.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/scene/CfgSceneWriter.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/write/CfgStreamWriter.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/write/CfgValueFormatter.cpp"
)
