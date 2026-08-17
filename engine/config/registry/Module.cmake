# @file engine/config/registry/Module.cmake
# @brief Configuration registry sources.

set(BLITZAR_CONFIG_REGISTRY_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/config/registry")
set(BLITZAR_CONFIG_REGISTRY_SOURCE_DIR "${BLITZAR_ROOT_DIR}/engine/config/registry")
set(BLITZAR_CONFIG_REGISTRY_SOURCES
    "${BLITZAR_CONFIG_REGISTRY_SOURCE_DIR}/CfgMain.cpp"
    "${BLITZAR_CONFIG_REGISTRY_SOURCE_DIR}/CfgApply.cpp"
    "${BLITZAR_CONFIG_REGISTRY_SOURCE_DIR}/CfgApplyScalar.cpp"
    "${BLITZAR_CONFIG_REGISTRY_SOURCE_DIR}/CfgApplyNormalized.cpp"
    "${BLITZAR_CONFIG_REGISTRY_SOURCE_DIR}/CfgEntries.cpp"
    "${BLITZAR_CONFIG_REGISTRY_SOURCE_DIR}/CfgEntriesCore.cpp"
    "${BLITZAR_CONFIG_REGISTRY_SOURCE_DIR}/CfgEntriesClient.cpp"
    "${BLITZAR_CONFIG_REGISTRY_SOURCE_DIR}/CfgEntriesInitState.cpp"
    "${BLITZAR_CONFIG_REGISTRY_SOURCE_DIR}/CfgEntriesFluid.cpp"
)
