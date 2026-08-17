/*
 * @file engine/physics/cuda/core/CudCore.inl
 * @project BLITZAR
 * @brief Ordered inclusion manifest for particle-system core responsibilities.
 */

/*
 * Fragment order is intentional: construction precedes configuration,
 * cosmology, and the remaining physical parameter accessors.
 */

#include "physics/cuda/core/CudConstruction.inl"
#include "physics/cuda/core/CudSolverConfiguration.inl"
#include "physics/cuda/core/CudCosmology.inl"
#include "physics/cuda/core/CudPhysicalParameters.inl"
