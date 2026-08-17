# @file engine/config/registry/Module.cmake
# @brief Configuration registry sources.

set(BLITZAR_CONFIG_REGISTRY_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/config/registry")
set(BLITZAR_CONFIG_REGISTRY_SOURCE_DIR "${BLITZAR_ROOT_DIR}/engine/config/registry")
set(BLITZAR_CONFIG_REGISTRY_SOURCES
    "${BLITZAR_ROOT_DIR}/engine/config/registry/runtime/CfgMain.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/registry/application/CfgApply.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/registry/application/CfgApplyScalar.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/registry/application/CfgApplyNormalized.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/registry/entries/CfgEntries.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/registry/entries/CfgEntriesCore.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/registry/entries/CfgEntriesClient.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/registry/entries/CfgEntriesInitState.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/registry/entries/CfgEntriesFluid.cpp"
)
