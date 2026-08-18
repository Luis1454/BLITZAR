# @file engine/physics/octree/Module.cmake
# @brief CPU octree implementation.

set(BLITZAR_PHYSICS_OCTREE_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/physics/octree")
set(BLITZAR_PHYSICS_OCTREE_SOURCE_DIR "${BLITZAR_ROOT_DIR}/engine/physics/octree")

set(BLITZAR_PHYSICS_OCTREE_SOURCES
    "${BLITZAR_PHYSICS_OCTREE_SOURCE_DIR}/force/OctreeForce.cpp"
    "${BLITZAR_PHYSICS_OCTREE_SOURCE_DIR}/model/Octree.cpp"
    "${BLITZAR_PHYSICS_OCTREE_SOURCE_DIR}/build/OctreeBuild.cpp"
    "${BLITZAR_PHYSICS_OCTREE_SOURCE_DIR}/traversal/OctreeTraversal.cpp"
    "${BLITZAR_PHYSICS_OCTREE_SOURCE_DIR}/model/OctParticleSystemHost.cpp"
    "${BLITZAR_PHYSICS_OCTREE_SOURCE_DIR}/model/OctHostMath.cpp"
    "${BLITZAR_PHYSICS_OCTREE_SOURCE_DIR}/force/OctForces.cpp"
    "${BLITZAR_PHYSICS_OCTREE_SOURCE_DIR}/adaptive/OctAdaptive.cpp"
    "${BLITZAR_PHYSICS_OCTREE_SOURCE_DIR}/model/OctCosmology.cpp"
    "${BLITZAR_PHYSICS_OCTREE_SOURCE_DIR}/model/OctConfiguration.cpp"
    "${BLITZAR_PHYSICS_OCTREE_SOURCE_DIR}/model/OctResources.cpp"
)
