#ifndef GRAVITY_ENGINE_INCLUDE_PHYSICS_OCTREE_HPP_
#define GRAVITY_ENGINE_INCLUDE_PHYSICS_OCTREE_HPP_

#include "physics/ForceLawPolicy.hpp"
#include "physics/Particle.hpp"
#include "physics/Vector.hpp"
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
    /// Description: Describes the octree operation contract.
    Octree();
    /// Description: Releases resources owned by Octree.
    ~Octree();

    /// Description: Describes the clear operation contract.
    void clear();
    /// Description: Describes the build operation contract.
    void build(const std::vector<Particle>& particles);
    /// Description: Describes the compute force on operation contract.
    Vector3 computeForceOn(const Particle& particle, std::size_t selfIndex,
                           const ForceLawPolicy& policy, OctreeOpeningCriterion criterion) const;
    /// Description: Describes the get node count operation contract.
    std::size_t getNodeCount() const;
    /// Description: Describes the get root index operation contract.
    int getRootIndex() const;
    /// Description: Describes the export gpu operation contract.
    void exportGpu(std::vector<GpuOctreeNode>& outNodes, std::vector<int>& outLeafIndices) const;

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

        /// Description: Describes the node operation contract.
        Node();
    };

    /// Description: Describes the build node recursive operation contract.
    int buildNodeRecursive(const std::vector<Particle>& particles, const std::vector<int>& indices,
                           const Vector3& center, float halfSize, int depth);
    /// Description: Describes the compute force recursive operation contract.
    Vector3 computeForceRecursive(const std::vector<Particle>& particles, int nodeIndex,
                                  const Particle& particle, std::size_t selfIndex,
                                  const ForceLawPolicy& policy,
                                  OctreeOpeningCriterion criterion) const;
    /// Description: Describes the child index for position operation contract.
    static int childIndexForPosition(const Vector3& position, const Vector3& center);
    /// Description: Describes the has children operation contract.
    static bool hasChildren(const Node& node);

    std::vector<Node> _nodes;
    std::optional<std::reference_wrapper<const std::vector<Particle>>> _particlesRef;
    int _root;
};


#endif // GRAVITY_ENGINE_INCLUDE_PHYSICS_OCTREE_HPP_
