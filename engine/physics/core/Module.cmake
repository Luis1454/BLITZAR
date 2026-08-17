# @file engine/physics/core/Module.cmake
# @brief Shared particle and force-law sources.

set(BLITZAR_PHYSICS_CORE_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/physics/core/include")
set(BLITZAR_PHYSICS_CORE_SOURCES
    "${BLITZAR_ROOT_DIR}/engine/physics/core/src/ForceLawPolicy.cpp"
    "${BLITZAR_ROOT_DIR}/engine/physics/core/src/ParticleHotData.cpp"
)
