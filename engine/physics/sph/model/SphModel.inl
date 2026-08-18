/*
 * @file engine/physics/sph/model/SphModel.inl
 * @project BLITZAR
 * @brief Shared SPH model parameters used by grid and kernel responsibilities.
 */

struct SphGridParams {
    int gridSize;
    int totalCells;
    float cellSize;
    float originX;
    float originY;
    float originZ;
};
