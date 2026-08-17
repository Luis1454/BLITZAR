# @file engine/config/registry/Module.cmake
# @brief Configuration registry sources.

set(BLITZAR_CONFIG_REGISTRY_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/config/registry/include")
set(BLITZAR_CONFIG_REGISTRY_SOURCE_DIR "${BLITZAR_ROOT_DIR}/engine/config/registry/src")
set(BLITZAR_CONFIG_REGISTRY_SOURCES
    "${BLITZAR_CONFIG_REGISTRY_SOURCE_DIR}/registry/Main.cpp"
    "${BLITZAR_CONFIG_REGISTRY_SOURCE_DIR}/registry/Apply.cpp"
    "${BLITZAR_CONFIG_REGISTRY_SOURCE_DIR}/registry/ApplyScalar.cpp"
    "${BLITZAR_CONFIG_REGISTRY_SOURCE_DIR}/registry/ApplyNormalized.cpp"
    "${BLITZAR_CONFIG_REGISTRY_SOURCE_DIR}/registry/Entries.cpp"
    "${BLITZAR_CONFIG_REGISTRY_SOURCE_DIR}/registry/EntriesCore.cpp"
    "${BLITZAR_CONFIG_REGISTRY_SOURCE_DIR}/registry/EntriesClient.cpp"
    "${BLITZAR_CONFIG_REGISTRY_SOURCE_DIR}/registry/EntriesInitState.cpp"
    "${BLITZAR_CONFIG_REGISTRY_SOURCE_DIR}/registry/EntriesFluid.cpp"
)
