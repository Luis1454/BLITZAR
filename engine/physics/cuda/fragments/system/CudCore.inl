/*
 * @file engine/physics/cuda/fragments/system/CudCore.inl
 * @project BLITZAR
 * @brief Ordered inclusion manifest for particle-system core responsibilities.
 */

/*
 * Fragment order is intentional: construction precedes configuration,
 * cosmology, and the remaining physical parameter accessors.
 */

#include "core/CudConstruction.inl"
#include "core/CudSolverConfiguration.inl"
#include "core/CudCosmology.inl"
#include "core/CudPhysicalParameters.inl"
