/*
 * @file engine/physics/treepm/graph/TpmGraph.hpp
 * @brief Host-side CUDA Graph capture contract for TreePM.
 */

#ifndef BLITZAR_ENGINE_PHYSICS_TREEPM_GRAPH_TPMGRAPH_HPP_
#define BLITZAR_ENGINE_PHYSICS_TREEPM_GRAPH_TPMGRAPH_HPP_

struct TpmGraphCaptureState final {
    bool captured = false;
    bool valid = false;
};

#endif // BLITZAR_ENGINE_PHYSICS_TREEPM_GRAPH_TPMGRAPH_HPP_
