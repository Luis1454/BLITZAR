# @file engine/config/env/Module.cmake
# @brief Platform environment configuration sources.

set(BLITZAR_CONFIG_ENV_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/config/env/include")
set(BLITZAR_CONFIG_ENV_SOURCE_DIR "${BLITZAR_ROOT_DIR}/engine/config/env/src")
if(WIN32)
    set(BLITZAR_CONFIG_ENV_SOURCES
        "${BLITZAR_CONFIG_ENV_SOURCE_DIR}/env/Base.cpp"
        "${BLITZAR_CONFIG_ENV_SOURCE_DIR}/env/Win.cpp"
    )
else()
    set(BLITZAR_CONFIG_ENV_SOURCES
        "${BLITZAR_CONFIG_ENV_SOURCE_DIR}/env/Base.cpp"
        "${BLITZAR_CONFIG_ENV_SOURCE_DIR}/env/Posix.cpp"
    )
endif()
