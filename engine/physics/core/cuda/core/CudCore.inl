/*
 * @file engine/physics/core/cuda/core/CudCore.inl
 * @project BLITZAR
 * @brief Ordered inclusion manifest for particle-system core responsibilities.
 */

/*
 * Fragment order is intentional: construction precedes configuration,
 * cosmology, and the remaining physical parameter accessors.
 */

#include "physics/core/cuda/core/CudConstruction.inl"
#include "physics/core/cuda/core/CudSolverConfiguration.inl"
#include "physics/core/cuda/core/CudCosmology.inl"
#include "physics/core/cuda/core/CudPhysicalParameters.inl"
