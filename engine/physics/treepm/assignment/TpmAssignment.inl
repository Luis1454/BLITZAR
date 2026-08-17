/*
 * @file engine/physics/treepm/assignment/TpmAssignment.inl
 * @brief Grid geometry, particle assignment, and field interpolation helpers.
 */

template <typename Scalar> struct Grid final {
    int size = 0;
    Scalar cellSize = static_cast<Scalar>(0.0);
    Scalar inverseCellSize = static_cast<Scalar>(0.0);
    Scalar originX = static_cast<Scalar>(0.0);
    Scalar originY = static_cast<Scalar>(0.0);
    Scalar originZ = static_cast<Scalar>(0.0);
    bool periodic = false;
};

int wrapGridIndex(int value, int size)
{
    const int wrapped = value % size;
    return wrapped < 0 ? wrapped + size : wrapped;
}

template <typename Scalar> int coordinate(Scalar value, Scalar origin, const Grid<Scalar>& grid)
{
    if (grid.periodic) {
        return wrapGridIndex(static_cast<int>(std::floor((value - origin) * grid.inverseCellSize)),
                             grid.size);
    }
    return std::clamp(static_cast<int>(std::floor((value - origin) * grid.inverseCellSize)), 0,
                      grid.size - 1);
}

template <typename Scalar> int cellHash(Vector3 position, const Grid<Scalar>& grid)
{
    return gridIndex(coordinate(static_cast<Scalar>(position.x), grid.originX, grid),
                     coordinate(static_cast<Scalar>(position.y), grid.originY, grid),
                     coordinate(static_cast<Scalar>(position.z), grid.originZ, grid), grid.size);
}

template <typename Scalar> Scalar assignmentWeight(Scalar distance, std::string_view assignment)
{
    const Scalar one = static_cast<Scalar>(1.0);
    const Scalar absDistance = std::fabs(distance);
    if (assignment == "tsc") {
        if (absDistance < static_cast<Scalar>(0.5)) {
            return static_cast<Scalar>(0.75) - absDistance * absDistance;
        }
        if (absDistance < static_cast<Scalar>(1.5)) {
            const Scalar tail = static_cast<Scalar>(1.5) - absDistance;
            return static_cast<Scalar>(0.5) * tail * tail;
        }
        return static_cast<Scalar>(0.0);
    }
    if (assignment == "pcs") {
        if (absDistance < one) {
            return (static_cast<Scalar>(4.0) -
                    static_cast<Scalar>(6.0) * absDistance * absDistance +
                    static_cast<Scalar>(3.0) * absDistance * absDistance * absDistance) /
                   static_cast<Scalar>(6.0);
        }
        if (absDistance < static_cast<Scalar>(2.0)) {
            const Scalar tail = static_cast<Scalar>(2.0) - absDistance;
            return tail * tail * tail / static_cast<Scalar>(6.0);
        }
        return static_cast<Scalar>(0.0);
    }
    return absDistance < one ? one - absDistance : static_cast<Scalar>(0.0);
}

template <typename Scalar>
int buildAssignmentStencil(Scalar coordinateValue, int gridSize, std::string_view assignment,
                           bool periodic, int (&indices)[4], Scalar (&weights)[4])
{
    if (assignment == "tsc") {
        const int center = static_cast<int>(std::floor(coordinateValue + static_cast<Scalar>(0.5)));
        for (int offset = -1; offset <= 1; ++offset) {
            const int index = offset + 1;
            const int raw = center + offset;
            indices[index] =
                periodic ? wrapGridIndex(raw, gridSize) : std::clamp(raw, 0, gridSize - 1);
            weights[index] =
                assignmentWeight(coordinateValue - static_cast<Scalar>(raw), assignment);
        }
        return 3;
    }
    if (assignment == "pcs") {
        const int center = static_cast<int>(std::floor(coordinateValue + static_cast<Scalar>(0.5)));
        for (int offset = -1; offset <= 2; ++offset) {
            const int index = offset + 1;
            const int raw = center + offset;
            indices[index] =
                periodic ? wrapGridIndex(raw, gridSize) : std::clamp(raw, 0, gridSize - 1);
            weights[index] =
                assignmentWeight(coordinateValue - static_cast<Scalar>(raw), assignment);
        }
        return 4;
    }
    const int rawLower = static_cast<int>(std::floor(coordinateValue));
    const int lower =
        periodic ? wrapGridIndex(rawLower, gridSize) : std::clamp(rawLower, 0, gridSize - 1);
    indices[0] = lower;
    indices[1] =
        periodic ? wrapGridIndex(rawLower + 1, gridSize) : std::min(lower + 1, gridSize - 1);
    const Scalar fraction = coordinateValue - static_cast<Scalar>(std::floor(coordinateValue));
    weights[0] = static_cast<Scalar>(1.0) - fraction;
    weights[1] = fraction;
    return 2;
}

template <typename Scalar> Scalar assignmentWindow(Scalar value, std::string_view assignment)
{
    const Scalar sincValue = sinc(value);
    const int power = assignment == "tsc" ? 6 : assignment == "pcs" ? 8 : 4;
    Scalar result = static_cast<Scalar>(1.0);
    for (int exponent = 0; exponent < power; ++exponent) {
        result *= sincValue;
    }
    return result;
}

template <typename Scalar>
Scalar sample(const std::vector<Scalar>& field, const Grid<Scalar>& grid, Vector3 position,
              std::string_view assignment)
{
    const Scalar rawX = (static_cast<Scalar>(position.x) - grid.originX) * grid.inverseCellSize;
    const Scalar rawY = (static_cast<Scalar>(position.y) - grid.originY) * grid.inverseCellSize;
    const Scalar rawZ = (static_cast<Scalar>(position.z) - grid.originZ) * grid.inverseCellSize;
    const Scalar sx = grid.periodic ? rawX - std::floor(rawX / static_cast<Scalar>(grid.size)) *
                                                 static_cast<Scalar>(grid.size)
                                    : std::clamp(rawX, static_cast<Scalar>(0.0),
                                                 static_cast<Scalar>(grid.size - 1));
    const Scalar sy = grid.periodic ? rawY - std::floor(rawY / static_cast<Scalar>(grid.size)) *
                                                 static_cast<Scalar>(grid.size)
                                    : std::clamp(rawY, static_cast<Scalar>(0.0),
                                                 static_cast<Scalar>(grid.size - 1));
    const Scalar sz = grid.periodic ? rawZ - std::floor(rawZ / static_cast<Scalar>(grid.size)) *
                                                 static_cast<Scalar>(grid.size)
                                    : std::clamp(rawZ, static_cast<Scalar>(0.0),
                                                 static_cast<Scalar>(grid.size - 1));
    int x[4] = {};
    int y[4] = {};
    int z[4] = {};
    Scalar wx[4] = {};
    Scalar wy[4] = {};
    Scalar wz[4] = {};
    const int xCount = buildAssignmentStencil(sx, grid.size, assignment, grid.periodic, x, wx);
    const int yCount = buildAssignmentStencil(sy, grid.size, assignment, grid.periodic, y, wy);
    const int zCount = buildAssignmentStencil(sz, grid.size, assignment, grid.periodic, z, wz);
    Scalar result = static_cast<Scalar>(0.0);
    for (int iz = 0; iz < zCount; ++iz) {
        for (int iy = 0; iy < yCount; ++iy) {
            for (int ix = 0; ix < xCount; ++ix) {
                result +=
                    field[gridIndex(x[ix], y[iy], z[iz], grid.size)] * wx[ix] * wy[iy] * wz[iz];
            }
        }
    }
    return result;
}

template <typename Scalar>
void depositParticle(const Particle& particle, const Grid<Scalar>& grid,
                     std::vector<Scalar>& density, std::string_view assignment)
{
    const Vector3 position = particle.getPosition();
    const Scalar rawX = (static_cast<Scalar>(position.x) - grid.originX) * grid.inverseCellSize;
    const Scalar rawY = (static_cast<Scalar>(position.y) - grid.originY) * grid.inverseCellSize;
    const Scalar rawZ = (static_cast<Scalar>(position.z) - grid.originZ) * grid.inverseCellSize;
    const Scalar sx = grid.periodic ? rawX - std::floor(rawX / static_cast<Scalar>(grid.size)) *
                                                 static_cast<Scalar>(grid.size)
                                    : std::clamp(rawX, static_cast<Scalar>(0.0),
                                                 static_cast<Scalar>(grid.size - 1));
    const Scalar sy = grid.periodic ? rawY - std::floor(rawY / static_cast<Scalar>(grid.size)) *
                                                 static_cast<Scalar>(grid.size)
                                    : std::clamp(rawY, static_cast<Scalar>(0.0),
                                                 static_cast<Scalar>(grid.size - 1));
    const Scalar sz = grid.periodic ? rawZ - std::floor(rawZ / static_cast<Scalar>(grid.size)) *
                                                 static_cast<Scalar>(grid.size)
                                    : std::clamp(rawZ, static_cast<Scalar>(0.0),
                                                 static_cast<Scalar>(grid.size - 1));
    int x[4] = {};
    int y[4] = {};
    int z[4] = {};
    Scalar wx[4] = {};
    Scalar wy[4] = {};
    Scalar wz[4] = {};
    const int xCount = buildAssignmentStencil(sx, grid.size, assignment, grid.periodic, x, wx);
    const int yCount = buildAssignmentStencil(sy, grid.size, assignment, grid.periodic, y, wy);
    const int zCount = buildAssignmentStencil(sz, grid.size, assignment, grid.periodic, z, wz);
    const Scalar densityScale =
        static_cast<Scalar>(particle.getMass()) / (grid.cellSize * grid.cellSize * grid.cellSize);
    for (int iz = 0; iz < zCount; ++iz) {
        for (int iy = 0; iy < yCount; ++iy) {
            for (int ix = 0; ix < xCount; ++ix) {
                density[gridIndex(x[ix], y[iy], z[iz], grid.size)] +=
                    densityScale * wx[ix] * wy[iy] * wz[iz];
            }
        }
    }
}
