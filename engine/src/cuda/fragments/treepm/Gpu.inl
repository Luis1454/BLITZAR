/*
 * @file engine/src/cuda/fragments/treepm/Gpu.inl
 * @project BLITZAR
 * @brief Ordered TreePM CUDA fragment composition.
 */

// Keep this order explicit: device helpers precede kernels, then runtime methods.
#include "fragments/treepm/Math.inl"
#include "fragments/treepm/Fft.inl"
#include "fragments/treepm/Bounds.inl"
#include "fragments/treepm/Deposit.inl"
#include "fragments/treepm/Neighbor.inl"
#include "fragments/treepm/LocalGrid.inl"
#include "fragments/treepm/TreeForce.inl"
#include "fragments/treepm/Layout.inl"
#include "fragments/treepm/Buffers.inl"
#include "fragments/treepm/NeighborGrid.inl"
#include "fragments/treepm/FftRuntime.inl"
#include "fragments/treepm/Graph.inl"
#include "fragments/treepm/Cosmology.inl"
#include "fragments/treepm/GridBuild.inl"
