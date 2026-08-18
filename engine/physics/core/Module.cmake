# @file engine/physics/core/Module.cmake
# @brief Core particle, force-law, and shared CUDA sources.

set(BLITZAR_PHYSICS_CORE_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/physics/core")
set(BLITZAR_PHYSICS_CORE_SOURCES
    "${BLITZAR_ROOT_DIR}/engine/physics/core/force/PhyForceLawPolicy.cpp"
    "${BLITZAR_ROOT_DIR}/engine/physics/core/particle/PhyParticleHotData.cpp"
)

set(BLITZAR_PHYSICS_CUDA_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/physics/core/cuda")
set(BLITZAR_PHYSICS_CUDA_SOURCE_DIR "${BLITZAR_ROOT_DIR}/engine/physics/core/cuda")
set(BLITZAR_PHYSICS_CUDA_HOST_SOURCES
    "${BLITZAR_PHYSICS_CUDA_SOURCE_DIR}/buffer/CudDeviceMemory.cpp"
)
set(BLITZAR_PHYSICS_CUDA_DEVICE_SOURCES
    "${BLITZAR_PHYSICS_CUDA_SOURCE_DIR}/buffer/CudDeviceMemory.cu"
    "${BLITZAR_PHYSICS_CUDA_SOURCE_DIR}/buffer/CudMemoryPool.cu"
    "${BLITZAR_PHYSICS_CUDA_SOURCE_DIR}/runtime/CudParticleSystem.cu"
)
