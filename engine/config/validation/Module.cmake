# @file engine/config/validation/Module.cmake
# @brief Configuration validation sources.

set(BLITZAR_CONFIG_VALIDATION_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/config/validation")
set(BLITZAR_CONFIG_VALIDATION_SOURCE_DIR "${BLITZAR_ROOT_DIR}/engine/config/validation")
set(BLITZAR_CONFIG_VALIDATION_SOURCES
    "${BLITZAR_ROOT_DIR}/engine/config/validation/physics/CfgPhysics.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/validation/render/CfgRender.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/validation/scenario/CfgScenario.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/validation/scenario/CfgScenarioCosmology.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/validation/scenario/CfgScenarioInitialState.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/validation/scenario/CfgScenarioRuntime.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/validation/scenario/CfgScenarioScene.cpp"
)
