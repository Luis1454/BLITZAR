/*
 * @file engine/include/physics/Octree.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Public physics interfaces and data contracts for deterministic simulation kernels.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_PHYSICS_OCTREE_HPP_
#define BLITZAR_ENGINE_INCLUDE_PHYSICS_OCTREE_HPP_

#include "physics/ForceLawPolicy.hpp"
#include "physics/Particle.hpp"
#include "physics/Vector.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

/*
 * @brief Defines the alignas type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
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

/*
 * @brief Defines the gpu octree node nav data type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct GpuOctreeNodeNavData {
    int nextIndex;
    std::uint8_t childMask;
    std::uint8_t reserved0;
    std::uint8_t reserved1;
    std::uint8_t reserved2;
};

/*
 * @brief Defines the gpu octree node type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
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

/*
 * @brief Defines the octree opening criterion type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
enum class OctreeOpeningCriterion {
    CenterOfMass,
    Bounds
};

/*
 * @brief Defines the octree type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class Octree {
public:
    Octree();
    /*
     * @brief Documents the ~octree operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    ~Octree();

    /*
     * @brief Documents the clear operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void clear();
    /*
     * @brief Documents the build operation contract.
     * @param particles Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void build(const std::vector<Particle>& particles);
    /*
     * @brief Documents the compute force on operation contract.
     * @param particle Input value used by this contract.
     * @param selfIndex Input value used by this contract.
     * @param policy Input value used by this contract.
     * @param criterion Input value used by this contract.
     * @return Vector3 value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    Vector3 computeForceOn(const Particle& particle, std::size_t selfIndex,
                           const ForceLawPolicy& policy, OctreeOpeningCriterion criterion) const;
    /*
     * @brief Documents the get node count operation contract.
     * @param None This contract does not take explicit parameters.
     * @return std::size_t value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    std::size_t getNodeCount() const;
    /*
     * @brief Documents the get root index operation contract.
     * @param None This contract does not take explicit parameters.
     * @return int value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    int getRootIndex() const;
    /*
     * @brief Documents the export gpu operation contract.
     * @param outNodes Input value used by this contract.
     * @param outLeafIndices Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void exportGpu(std::vector<GpuOctreeNode>& outNodes, std::vector<int>& outLeafIndices) const;

private:
    /*
     * @brief Defines the node type contract.
     * @param None This contract does not take explicit parameters.
     * @return Not applicable; this block documents a type contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    struct Node {
        Vector3 center;
        float halfSize;
        float mass;
        Vector3 centerOfMass;
        std::array<int, 8> children;
        unsigned char childMask;
        std::vector<int> particleIndices;

        Node();
    };

    /*
     * @brief Documents the build node recursive operation contract.
     * @param particles Input value used by this contract.
     * @param indices Input value used by this contract.
     * @param center Input value used by this contract.
     * @param halfSize Input value used by this contract.
     * @param depth Input value used by this contract.
     * @return int value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    int buildNodeRecursive(const std::vector<Particle>& particles, const std::vector<int>& indices,
                           const Vector3& center, float halfSize, int depth);
    /*
     * @brief Documents the compute force recursive operation contract.
     * @param particles Input value used by this contract.
     * @param nodeIndex Input value used by this contract.
     * @param particle Input value used by this contract.
     * @param selfIndex Input value used by this contract.
     * @param policy Input value used by this contract.
     * @param criterion Input value used by this contract.
     * @return Vector3 value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    Vector3 computeForceRecursive(const std::vector<Particle>& particles, int nodeIndex,
                                  const Particle& particle, std::size_t selfIndex,
                                  const ForceLawPolicy& policy,
                                  OctreeOpeningCriterion criterion) const;
    /*
     * @brief Documents the child index for position operation contract.
     * @param position Input value used by this contract.
     * @param center Input value used by this contract.
     * @return int value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    static int childIndexForPosition(const Vector3& position, const Vector3& center);
    /*
     * @brief Documents the has children operation contract.
     * @param node Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    static bool hasChildren(const Node& node);

    std::vector<Node> _nodes;
    std::optional<std::reference_wrapper<const std::vector<Particle>>> _particlesRef;
    int _root;
};


#endif // BLITZAR_ENGINE_INCLUDE_PHYSICS_OCTREE_HPP_
