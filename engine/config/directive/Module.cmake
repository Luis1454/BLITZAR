# @file engine/config/directive/Module.cmake
# @brief Directive parser and writer sources.

set(BLITZAR_CONFIG_DIRECTIVE_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/config/directive/include")
set(BLITZAR_CONFIG_DIRECTIVE_SOURCE_DIR "${BLITZAR_ROOT_DIR}/engine/config/directive/src")
set(BLITZAR_CONFIG_DIRECTIVE_SOURCES
    "${BLITZAR_CONFIG_DIRECTIVE_SOURCE_DIR}/directive/Config.cpp"
    "${BLITZAR_CONFIG_DIRECTIVE_SOURCE_DIR}/directive/Options.cpp"
    "${BLITZAR_CONFIG_DIRECTIVE_SOURCE_DIR}/directive/Parser.cpp"
    "${BLITZAR_CONFIG_DIRECTIVE_SOURCE_DIR}/directive/Scene.cpp"
    "${BLITZAR_CONFIG_DIRECTIVE_SOURCE_DIR}/directive/LegacyScene.cpp"
    "${BLITZAR_CONFIG_DIRECTIVE_SOURCE_DIR}/directive/Write.cpp"
    "${BLITZAR_CONFIG_DIRECTIVE_SOURCE_DIR}/directive/CfgCore.cpp"
    "${BLITZAR_CONFIG_DIRECTIVE_SOURCE_DIR}/directive/InitialState.cpp"
    "${BLITZAR_CONFIG_DIRECTIVE_SOURCE_DIR}/directive/SceneWriter.cpp"
    "${BLITZAR_CONFIG_DIRECTIVE_SOURCE_DIR}/directive/StreamWriter.cpp"
    "${BLITZAR_CONFIG_DIRECTIVE_SOURCE_DIR}/directive/ValueFormatter.cpp"
)
