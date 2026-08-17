# @file engine/physics/core/Module.cmake
# @brief Shared particle and force-law sources.

set(BLITZAR_PHYSICS_CORE_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/physics/core")
set(BLITZAR_PHYSICS_CORE_SOURCES
    "${BLITZAR_ROOT_DIR}/engine/physics/core/force/PhyForceLawPolicy.cpp"
    "${BLITZAR_ROOT_DIR}/engine/physics/core/particle/PhyParticleHotData.cpp"
)
