# @file engine/config/validation/Module.cmake
# @brief Configuration validation sources.

set(BLITZAR_CONFIG_VALIDATION_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/config/validation")
set(BLITZAR_CONFIG_VALIDATION_SOURCE_DIR "${BLITZAR_ROOT_DIR}/engine/config/validation")
set(BLITZAR_CONFIG_VALIDATION_SOURCES
    "${BLITZAR_CONFIG_VALIDATION_SOURCE_DIR}/CfgPhysics.cpp"
    "${BLITZAR_CONFIG_VALIDATION_SOURCE_DIR}/CfgRender.cpp"
    "${BLITZAR_CONFIG_VALIDATION_SOURCE_DIR}/CfgScenario.cpp"
    "${BLITZAR_CONFIG_VALIDATION_SOURCE_DIR}/CfgScenarioCosmology.cpp"
    "${BLITZAR_CONFIG_VALIDATION_SOURCE_DIR}/CfgScenarioInitialState.cpp"
    "${BLITZAR_CONFIG_VALIDATION_SOURCE_DIR}/CfgScenarioRuntime.cpp"
    "${BLITZAR_CONFIG_VALIDATION_SOURCE_DIR}/CfgScenarioScene.cpp"
)
