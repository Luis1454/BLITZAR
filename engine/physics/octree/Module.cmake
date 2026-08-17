# @file engine/physics/octree/Module.cmake
# @brief CPU octree implementation.

set(BLITZAR_PHYSICS_OCTREE_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/physics/octree/include")
set(BLITZAR_PHYSICS_OCTREE_SOURCE_DIR "${BLITZAR_ROOT_DIR}/engine/physics/octree/src")

set(BLITZAR_PHYSICS_OCTREE_SOURCES
    "${BLITZAR_PHYSICS_OCTREE_SOURCE_DIR}/OctreeForce.cpp"
    "${BLITZAR_PHYSICS_OCTREE_SOURCE_DIR}/Octree.cpp"
    "${BLITZAR_PHYSICS_OCTREE_SOURCE_DIR}/OctreeBuild.cpp"
    "${BLITZAR_PHYSICS_OCTREE_SOURCE_DIR}/OctreeTraversal.cpp"
    "${BLITZAR_PHYSICS_OCTREE_SOURCE_DIR}/ParticleSystemHost.cpp"
    "${BLITZAR_PHYSICS_OCTREE_SOURCE_DIR}/OctHostMath.cpp"
    "${BLITZAR_PHYSICS_OCTREE_SOURCE_DIR}/OctForces.cpp"
    "${BLITZAR_PHYSICS_OCTREE_SOURCE_DIR}/OctAdaptive.cpp"
    "${BLITZAR_PHYSICS_OCTREE_SOURCE_DIR}/OctCosmology.cpp"
    "${BLITZAR_PHYSICS_OCTREE_SOURCE_DIR}/OctConfiguration.cpp"
    "${BLITZAR_PHYSICS_OCTREE_SOURCE_DIR}/OctResources.cpp"
)
