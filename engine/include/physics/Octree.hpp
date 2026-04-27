#ifndef GRAVITY_ENGINE_INCLUDE_PHYSICS_OCTREE_HPP_
#define GRAVITY_ENGINE_INCLUDE_PHYSICS_OCTREE_HPP_

#include "physics/ForceLawPolicy.hpp"
#include "physics/Vector.hpp"
#include "physics/Particle.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

/// Description: Defines the alignas data or behavior contract.
struct alignas(32) GpuOctreeNodeHotData {
    float centerX;
    float centerY;
    float centerZ;
    float halfSize;
    float mass;
    float comX;
    float comY;
    float comZ;
};

/// Description: Defines the GpuOctreeNodeNavData data or behavior contract.
struct GpuOctreeNodeNavData {
    int nextIndex;
    std::uint8_t childMask;
    std::uint8_t reserved0;
    std::uint8_t reserved1;
    std::uint8_t reserved2;
};

/// Description: Defines the GpuOctreeNode data or behavior contract.
struct GpuOctreeNode {
    float centerX;
    float centerY;
    float centerZ;
    float halfSize;
    float mass;
    float comX;
    float comY;
    float comZ;
    int children[8];
    int leafStart;
    int leafCount;
    int parentIndex;
    int nextIndex;
    unsigned int childMask;
};

/// Description: Enumerates the supported OctreeOpeningCriterion values.
enum class OctreeOpeningCriterion {
    CenterOfMass,
    Bounds
};

/// Description: Defines the Octree data or behavior contract.
class Octree {
    public:
        /// Description: Executes the Octree operation.
        Octree();
        /// Description: Releases resources owned by Octree.
        ~Octree();

        /// Description: Executes the clear operation.
        void clear();
        /// Description: Executes the build operation.
        void build(const std::vector<Particle> &particles);
        /// Description: Executes the computeForceOn operation.
        Vector3 computeForceOn(const Particle &particle, std::size_t selfIndex, const ForceLawPolicy &policy, OctreeOpeningCriterion criterion) const;
        /// Description: Executes the getNodeCount operation.
        std::size_t getNodeCount() const;
        /// Description: Executes the getRootIndex operation.
        int getRootIndex() const;
        /// Description: Executes the exportGpu operation.
        void exportGpu(std::vector<GpuOctreeNode> &outNodes, std::vector<int> &outLeafIndices) const;

    private:
        /// Description: Defines the Node data or behavior contract.
        struct Node {
            Vector3 center;
            float halfSize;
            float mass;
            Vector3 centerOfMass;
            std::array<int, 8> children;
            unsigned char childMask;
            std::vector<int> particleIndices;

            /// Description: Executes the Node operation.
            Node();
        };

        /// Description: Executes the buildNodeRecursive operation.
        int buildNodeRecursive(const std::vector<Particle> &particles, const std::vector<int> &indices, const Vector3 &center, float halfSize, int depth);
        Vector3 computeForceRecursive(
            const std::vector<Particle> &particles,
            int nodeIndex,
            const Particle &particle,
            std::size_t selfIndex,
            const ForceLawPolicy &policy,
            OctreeOpeningCriterion criterion
        ) const;
        /// Description: Executes the childIndexForPosition operation.
        static int childIndexForPosition(const Vector3 &position, const Vector3 &center);
        /// Description: Executes the hasChildren operation.
        static bool hasChildren(const Node &node);

        std::vector<Node> _nodes;
        std::optional<std::reference_wrapper<const std::vector<Particle>>> _particlesRef;
        int _root;
};




#endif // GRAVITY_ENGINE_INCLUDE_PHYSICS_OCTREE_HPP_
