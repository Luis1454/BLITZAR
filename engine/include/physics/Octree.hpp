#pragma once

#include "physics/Vector.hpp"
#include "physics/Particle.hpp"
#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

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
    unsigned char childMask;
};

class Octree {
    public:
        Octree();
        ~Octree();

        void clear();
        void build(const std::vector<Particle> &particles);
        Vector3 computeForceOn(const Particle &particle, std::size_t selfIndex, float theta, float softening) const;
        std::size_t getNodeCount() const;
        int getRootIndex() const;
        void exportGpu(std::vector<GpuOctreeNode> &outNodes, std::vector<int> &outLeafIndices) const;

    private:
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

        int buildNodeRecursive(const std::vector<Particle> &particles, const std::vector<int> &indices, const Vector3 &center, float halfSize, int depth);
        Vector3 computeForceRecursive(
            const std::vector<Particle> &particles,
            int nodeIndex,
            const Particle &particle,
            std::size_t selfIndex,
            float theta,
            float softening
        ) const;
        static int childIndexForPosition(const Vector3 &position, const Vector3 &center);
        static bool hasChildren(const Node &node);

        std::vector<Node> _nodes;
        std::optional<std::reference_wrapper<const std::vector<Particle>>> _particlesRef;
        int _root;
};



