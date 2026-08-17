# @file engine/config/validation/Module.cmake
# @brief Configuration validation sources.

set(BLITZAR_CONFIG_VALIDATION_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/config/validation/include")
set(BLITZAR_CONFIG_VALIDATION_SOURCE_DIR "${BLITZAR_ROOT_DIR}/engine/config/validation/src")
set(BLITZAR_CONFIG_VALIDATION_SOURCES
    "${BLITZAR_CONFIG_VALIDATION_SOURCE_DIR}/validation/Physics.cpp"
    "${BLITZAR_CONFIG_VALIDATION_SOURCE_DIR}/validation/Render.cpp"
    "${BLITZAR_CONFIG_VALIDATION_SOURCE_DIR}/validation/Scenario.cpp"
    "${BLITZAR_CONFIG_VALIDATION_SOURCE_DIR}/validation/ScenarioCosmology.cpp"
    "${BLITZAR_CONFIG_VALIDATION_SOURCE_DIR}/validation/ScenarioInitialState.cpp"
    "${BLITZAR_CONFIG_VALIDATION_SOURCE_DIR}/validation/ScenarioRuntime.cpp"
    "${BLITZAR_CONFIG_VALIDATION_SOURCE_DIR}/validation/ScenarioScene.cpp"
)
