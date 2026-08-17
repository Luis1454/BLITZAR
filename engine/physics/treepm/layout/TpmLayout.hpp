/*
 * @file engine/physics/treepm/layout/TpmLayout.hpp
 * @brief Host-side particle layout contract for TreePM.
 */

#ifndef BLITZAR_ENGINE_PHYSICS_TREEPM_LAYOUT_TPMLAYOUT_HPP_
#define BLITZAR_ENGINE_PHYSICS_TREEPM_LAYOUT_TPMLAYOUT_HPP_

enum class TpmLayoutMode {
    Linear = 0,
    Morton = 1,
    GatherMorton = 2,
};

#endif // BLITZAR_ENGINE_PHYSICS_TREEPM_LAYOUT_TPMLAYOUT_HPP_
