# @file engine/config/args/Module.cmake
# @brief Command-line argument sources.

set(BLITZAR_CONFIG_ARGS_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/config/args")
set(BLITZAR_CONFIG_ARGS_SOURCE_DIR "${BLITZAR_ROOT_DIR}/engine/config/args")
set(BLITZAR_CONFIG_ARGS_SOURCES
    "${BLITZAR_ROOT_DIR}/engine/config/args/parsing/CfgMain.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/args/parsing/CfgParse.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/args/options/CfgCoreOptions.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/args/options/CfgClientOptions.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/args/options/CfgInitOptions.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/args/options/CfgInitStateOptions.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/args/options/CfgFluidOptions.cpp"
)
set(BLITZAR_CONFIG_ARGS_COMMAND_SOURCES
    "${BLITZAR_ROOT_DIR}/engine/config/args/parsing/CfgParse.cpp"
)
