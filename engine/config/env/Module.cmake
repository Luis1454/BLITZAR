# @file engine/config/env/Module.cmake
# @brief Platform environment configuration sources.

set(BLITZAR_CONFIG_ENV_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/config/env")
set(BLITZAR_CONFIG_ENV_SOURCE_DIR "${BLITZAR_ROOT_DIR}/engine/config/env")
if(WIN32)
    set(BLITZAR_CONFIG_ENV_SOURCES
        "${BLITZAR_ROOT_DIR}/engine/config/env/platform/CfgBase.cpp"
        "${BLITZAR_ROOT_DIR}/engine/config/env/platform/CfgWin.cpp"
    )
else()
    set(BLITZAR_CONFIG_ENV_SOURCES
        "${BLITZAR_ROOT_DIR}/engine/config/env/platform/CfgBase.cpp"
        "${BLITZAR_ROOT_DIR}/engine/config/env/platform/CfgPosix.cpp"
    )
endif()
