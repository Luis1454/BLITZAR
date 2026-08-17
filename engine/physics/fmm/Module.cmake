# @file engine/physics/fmm/Module.cmake
# @brief CPU FMM implementation and qualification sources.

set(BLITZAR_PHYSICS_FMM_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/physics/fmm/include")
set(BLITZAR_PHYSICS_FMM_SOURCES
    "${BLITZAR_ROOT_DIR}/engine/physics/fmm/src/FmmBuild.cpp"
    "${BLITZAR_ROOT_DIR}/engine/physics/fmm/src/FmmEvaluate.cpp"
    "${BLITZAR_ROOT_DIR}/engine/physics/fmm/src/FmmMetrics.cpp"
)
