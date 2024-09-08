/*
** EPITECH PROJECT, 2024
** rtxcpp
** File description:
** Octree
*/

#ifndef OCTREE_HPP_
#define OCTREE_HPP_

#include "Vector.hpp"
#include "Particle.hpp"

class Octree {
    public:
        Octree();
        ~Octree();

        void insert(Particle particle);

        void setChild(int idx, Octree *child);
        void setCenter(Vector3 center);
        void setSize(float size);

        Vector3 getCenter();
        float getSize();

    private:
        std::vector<Particle> _particles;
        Octree *_children[8];
        Vector3 _center;
        float _size;
};

#endif /* !OCTREE_HPP_ */
