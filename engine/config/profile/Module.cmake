# @file engine/config/profile/Module.cmake
# @brief Runtime profile configuration sources.

set(BLITZAR_CONFIG_PROFILE_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/config/profile")
set(BLITZAR_CONFIG_PROFILE_SOURCE_DIR "${BLITZAR_ROOT_DIR}/engine/config/profile")
set(BLITZAR_CONFIG_PROFILE_SOURCES
    "${BLITZAR_ROOT_DIR}/engine/config/profile/profile/CfgMain.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/profile/profile/CfgPerformance.cpp"
)
set(BLITZAR_CONFIG_PROFILE_COMMAND_SOURCES
    "${BLITZAR_ROOT_DIR}/engine/config/profile/profile/CfgPerformance.cpp"
)
