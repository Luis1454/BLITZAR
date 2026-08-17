# @file engine/physics/fmm/Module.cmake
# @brief CPU FMM implementation and qualification sources.

set(BLITZAR_PHYSICS_FMM_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/physics/fmm")
set(BLITZAR_PHYSICS_FMM_SOURCES
    "${BLITZAR_ROOT_DIR}/engine/physics/fmm/FmmBuild.cpp"
    "${BLITZAR_ROOT_DIR}/engine/physics/fmm/FmmEvaluate.cpp"
    "${BLITZAR_ROOT_DIR}/engine/physics/fmm/FmmMetrics.cpp"
)
