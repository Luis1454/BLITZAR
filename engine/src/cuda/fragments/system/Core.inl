/*
 * @file engine/src/cuda/fragments/system/Core.inl
 * @project BLITZAR
 * @brief Ordered inclusion manifest for particle-system core responsibilities.
 */

/*
 * Fragment order is intentional: construction precedes configuration,
 * cosmology, and the remaining physical parameter accessors.
 */

#include "core/Construction.inl"
#include "core/SolverConfiguration.inl"
#include "core/Cosmology.inl"
#include "core/PhysicalParameters.inl"
