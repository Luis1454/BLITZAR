# @file engine/config/args/Module.cmake
# @brief Command-line argument sources.

set(BLITZAR_CONFIG_ARGS_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/config/args/include")
set(BLITZAR_CONFIG_ARGS_SOURCE_DIR "${BLITZAR_ROOT_DIR}/engine/config/args/src")
set(BLITZAR_CONFIG_ARGS_SOURCES
    "${BLITZAR_CONFIG_ARGS_SOURCE_DIR}/args/Main.cpp"
    "${BLITZAR_CONFIG_ARGS_SOURCE_DIR}/args/Parse.cpp"
    "${BLITZAR_CONFIG_ARGS_SOURCE_DIR}/args/CoreOptions.cpp"
    "${BLITZAR_CONFIG_ARGS_SOURCE_DIR}/args/ClientOptions.cpp"
    "${BLITZAR_CONFIG_ARGS_SOURCE_DIR}/args/InitOptions.cpp"
    "${BLITZAR_CONFIG_ARGS_SOURCE_DIR}/args/InitStateOptions.cpp"
    "${BLITZAR_CONFIG_ARGS_SOURCE_DIR}/args/FluidOptions.cpp"
)
set(BLITZAR_CONFIG_ARGS_COMMAND_SOURCES
    "${BLITZAR_CONFIG_ARGS_SOURCE_DIR}/args/Parse.cpp"
)
