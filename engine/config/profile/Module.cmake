# @file engine/config/profile/Module.cmake
# @brief Runtime profile configuration sources.

set(BLITZAR_CONFIG_PROFILE_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/config/profile/include")
set(BLITZAR_CONFIG_PROFILE_SOURCE_DIR "${BLITZAR_ROOT_DIR}/engine/config/profile/src")
set(BLITZAR_CONFIG_PROFILE_SOURCES
    "${BLITZAR_CONFIG_PROFILE_SOURCE_DIR}/profile/Main.cpp"
    "${BLITZAR_CONFIG_PROFILE_SOURCE_DIR}/profile/Performance.cpp"
)
set(BLITZAR_CONFIG_PROFILE_COMMAND_SOURCES
    "${BLITZAR_CONFIG_PROFILE_SOURCE_DIR}/profile/Performance.cpp"
)
