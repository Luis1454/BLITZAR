# @file engine/config/args/Module.cmake
# @brief Command-line argument sources.

set(BLITZAR_CONFIG_ARGS_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/config/args")
set(BLITZAR_CONFIG_ARGS_SOURCE_DIR "${BLITZAR_ROOT_DIR}/engine/config/args")
set(BLITZAR_CONFIG_ARGS_SOURCES
    "${BLITZAR_CONFIG_ARGS_SOURCE_DIR}/CfgMain.cpp"
    "${BLITZAR_CONFIG_ARGS_SOURCE_DIR}/CfgParse.cpp"
    "${BLITZAR_CONFIG_ARGS_SOURCE_DIR}/CfgCoreOptions.cpp"
    "${BLITZAR_CONFIG_ARGS_SOURCE_DIR}/CfgClientOptions.cpp"
    "${BLITZAR_CONFIG_ARGS_SOURCE_DIR}/CfgInitOptions.cpp"
    "${BLITZAR_CONFIG_ARGS_SOURCE_DIR}/CfgInitStateOptions.cpp"
    "${BLITZAR_CONFIG_ARGS_SOURCE_DIR}/CfgFluidOptions.cpp"
)
set(BLITZAR_CONFIG_ARGS_COMMAND_SOURCES
    "${BLITZAR_CONFIG_ARGS_SOURCE_DIR}/CfgParse.cpp"
)
