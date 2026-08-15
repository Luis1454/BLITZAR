/*
 * @file engine/src/physics/TreePmCpu.cpp
 * @brief Deterministic CPU TreePM field construction and sampling.
 */

#include "TreePmCpu.hpp"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <numeric>
#include <omp.h>
#include <string_view>
#include <utility>

namespace blitzar_physics_tree_pm_cpu {
template <typename Scalar>
constexpr Scalar kTwoPi = static_cast<Scalar>(6.2831853071795864769);

template <typename Scalar>
constexpr Scalar kFourPi = static_cast<Scalar>(12.566370614359172);

int gridIndex(int x, int y, int z, int gridSize)
{
    return (z * gridSize + y) * gridSize + x;
}

template <typename Scalar>
Scalar modifiedBesselK1(Scalar value)
{
    const Scalar x = std::max(value, static_cast<Scalar>(1.0e-4));
    if (x <= static_cast<Scalar>(2.0)) {
        const Scalar y = x * x * static_cast<Scalar>(0.25);
        Scalar i1Poly = static_cast<Scalar>(0.00032411);
        i1Poly = static_cast<Scalar>(0.00301532) + y * i1Poly;
        i1Poly = static_cast<Scalar>(0.02658733) + y * i1Poly;
        i1Poly = static_cast<Scalar>(0.15084934) + y * i1Poly;
        i1Poly = static_cast<Scalar>(0.51498869) + y * i1Poly;
        i1Poly = static_cast<Scalar>(0.87890594) + y * i1Poly;
        const Scalar i1 = x * static_cast<Scalar>(0.5) *
                          (static_cast<Scalar>(1.0) + y * i1Poly);
        Scalar k1Poly = static_cast<Scalar>(-0.00004686);
        k1Poly = static_cast<Scalar>(-0.00110404) + y * k1Poly;
        k1Poly = static_cast<Scalar>(-0.01919402) + y * k1Poly;
        k1Poly = static_cast<Scalar>(-0.18156897) + y * k1Poly;
        k1Poly = static_cast<Scalar>(-0.67278579) + y * k1Poly;
        k1Poly = static_cast<Scalar>(0.15443144) + y * k1Poly;
        return std::log(x * static_cast<Scalar>(0.5)) * i1 +
               (static_cast<Scalar>(1.0) / x) *
               (static_cast<Scalar>(1.0) + y * k1Poly);
    }
    const Scalar y = static_cast<Scalar>(2.0) / x;
    Scalar asymptotic = static_cast<Scalar>(-0.00068245);
    asymptotic = static_cast<Scalar>(0.00325614) + y * asymptotic;
    asymptotic = static_cast<Scalar>(-0.00780353) + y * asymptotic;
    asymptotic = static_cast<Scalar>(0.01504268) + y * asymptotic;
    asymptotic = static_cast<Scalar>(-0.03655620) + y * asymptotic;
    asymptotic = static_cast<Scalar>(0.23498619) + y * asymptotic;
    return std::exp(-x) * (static_cast<Scalar>(1.0) / std::sqrt(x)) *
           static_cast<Scalar>(1.25331414) *
           (static_cast<Scalar>(1.0) + y * asymptotic);
}

template <typename Scalar>
Scalar sinc(Scalar value)
{
    return std::fabs(value) < static_cast<Scalar>(1.0e-5)
               ? static_cast<Scalar>(1.0)
               : std::sin(value) / value;
}

template <typename Scalar>
void fft1d(std::vector<std::complex<Scalar>>& values, int start, int stride, int size, bool inverse)
{
    for (int i = 1, j = 0; i < size; ++i) {
        int bit = size >> 1;
        for (; (j & bit) != 0; bit >>= 1) {
            j ^= bit;
        }
        j ^= bit;
        if (i < j) {
            std::swap(values[static_cast<std::size_t>(start + i * stride)],
                      values[static_cast<std::size_t>(start + j * stride)]);
        }
    }

    for (int length = 2; length <= size; length <<= 1) {
        const Scalar angle = (inverse ? static_cast<Scalar>(1.0) : static_cast<Scalar>(-1.0)) *
                             kTwoPi<Scalar> / static_cast<Scalar>(length);
        const std::complex<Scalar> step(std::cos(angle), std::sin(angle));
        for (int offset = 0; offset < size; offset += length) {
            std::complex<Scalar> factor(static_cast<Scalar>(1.0), static_cast<Scalar>(0.0));
            const int halfLength = length >> 1;
            for (int i = 0; i < halfLength; ++i) {
                const std::size_t evenIndex = static_cast<std::size_t>(start +
                                                                         (offset + i) * stride);
                const std::size_t oddIndex = static_cast<std::size_t>(start +
                                                                        (offset + i + halfLength) *
                                                                        stride);
                const std::complex<Scalar> even = values[evenIndex];
                const std::complex<Scalar> odd = factor * values[oddIndex];
                values[evenIndex] = even + odd;
                values[oddIndex] = even - odd;
                factor *= step;
            }
        }
    }
}

template <typename Scalar>
void fft3d(std::vector<std::complex<Scalar>>& values, int size, bool inverse)
{
#pragma omp parallel for schedule(static)
    for (int z = 0; z < size; ++z) {
        for (int y = 0; y < size; ++y) {
            fft1d<Scalar>(values, gridIndex(0, y, z, size), 1, size, inverse);
        }
    }
#pragma omp parallel for schedule(static)
    for (int z = 0; z < size; ++z) {
        for (int x = 0; x < size; ++x) {
            fft1d<Scalar>(values, gridIndex(x, 0, z, size), size, size, inverse);
        }
    }
#pragma omp parallel for schedule(static)
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            fft1d<Scalar>(values, gridIndex(x, y, 0, size), size * size, size, inverse);
        }
    }
    if (inverse) {
        const Scalar inverseCells = static_cast<Scalar>(1.0) /
                                    static_cast<Scalar>(size * size * size);
        for (std::complex<Scalar>& value : values) {
            value *= inverseCells;
        }
    }
}

template <typename Scalar>
struct Grid final {
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

template <typename Scalar>
int coordinate(Scalar value, Scalar origin, const Grid<Scalar>& grid)
{
    if (grid.periodic) {
        return wrapGridIndex(static_cast<int>(std::floor((value - origin) * grid.inverseCellSize)),
                             grid.size);
    }
    return std::clamp(static_cast<int>(std::floor((value - origin) * grid.inverseCellSize)),
                      0, grid.size - 1);
}

template <typename Scalar>
int cellHash(Vector3 position, const Grid<Scalar>& grid)
{
    return gridIndex(coordinate(static_cast<Scalar>(position.x), grid.originX, grid),
                     coordinate(static_cast<Scalar>(position.y), grid.originY, grid),
                     coordinate(static_cast<Scalar>(position.z), grid.originZ, grid), grid.size);
}

template <typename Scalar>
Scalar assignmentWeight(Scalar distance, std::string_view assignment)
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
            return (static_cast<Scalar>(4.0) - static_cast<Scalar>(6.0) * absDistance * absDistance +
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
                           bool periodic,
                           int (&indices)[4], Scalar (&weights)[4])
{
    if (assignment == "tsc") {
        const int center = static_cast<int>(std::floor(coordinateValue + static_cast<Scalar>(0.5)));
        for (int offset = -1; offset <= 1; ++offset) {
            const int index = offset + 1;
            const int raw = center + offset;
            indices[index] = periodic ? wrapGridIndex(raw, gridSize) : std::clamp(raw, 0, gridSize - 1);
            weights[index] = assignmentWeight(coordinateValue - static_cast<Scalar>(raw), assignment);
        }
        return 3;
    }
    if (assignment == "pcs") {
        const int center = static_cast<int>(std::floor(coordinateValue + static_cast<Scalar>(0.5)));
        for (int offset = -1; offset <= 2; ++offset) {
            const int index = offset + 1;
            const int raw = center + offset;
            indices[index] = periodic ? wrapGridIndex(raw, gridSize) : std::clamp(raw, 0, gridSize - 1);
            weights[index] = assignmentWeight(coordinateValue - static_cast<Scalar>(raw), assignment);
        }
        return 4;
    }
    const int rawLower = static_cast<int>(std::floor(coordinateValue));
    const int lower = periodic ? wrapGridIndex(rawLower, gridSize) : std::clamp(rawLower, 0, gridSize - 1);
    indices[0] = lower;
    indices[1] = periodic ? wrapGridIndex(rawLower + 1, gridSize) : std::min(lower + 1, gridSize - 1);
    const Scalar fraction = coordinateValue - static_cast<Scalar>(std::floor(coordinateValue));
    weights[0] = static_cast<Scalar>(1.0) - fraction;
    weights[1] = fraction;
    return 2;
}

template <typename Scalar>
Scalar assignmentWindow(Scalar value, std::string_view assignment)
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
    const Scalar sx = grid.periodic
                          ? rawX - std::floor(rawX / static_cast<Scalar>(grid.size)) *
                                       static_cast<Scalar>(grid.size)
                          : std::clamp(rawX, static_cast<Scalar>(0.0),
                                       static_cast<Scalar>(grid.size - 1));
    const Scalar sy = grid.periodic ? rawY - std::floor(rawY / static_cast<Scalar>(grid.size)) * static_cast<Scalar>(grid.size) : std::clamp(rawY,
                                     static_cast<Scalar>(0.0),
                                     static_cast<Scalar>(grid.size - 1));
    const Scalar sz = grid.periodic ? rawZ - std::floor(rawZ / static_cast<Scalar>(grid.size)) * static_cast<Scalar>(grid.size) : std::clamp(rawZ,
                                     static_cast<Scalar>(0.0),
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
                result += field[gridIndex(x[ix], y[iy], z[iz], grid.size)] *
                          wx[ix] * wy[iy] * wz[iz];
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
    const Scalar sx = grid.periodic ? rawX - std::floor(rawX / static_cast<Scalar>(grid.size)) * static_cast<Scalar>(grid.size) : std::clamp(rawX,
                                     static_cast<Scalar>(0.0),
                                     static_cast<Scalar>(grid.size - 1));
    const Scalar sy = grid.periodic ? rawY - std::floor(rawY / static_cast<Scalar>(grid.size)) * static_cast<Scalar>(grid.size) : std::clamp(rawY,
                                     static_cast<Scalar>(0.0),
                                     static_cast<Scalar>(grid.size - 1));
    const Scalar sz = grid.periodic ? rawZ - std::floor(rawZ / static_cast<Scalar>(grid.size)) * static_cast<Scalar>(grid.size) : std::clamp(rawZ,
                                     static_cast<Scalar>(0.0),
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
    const Scalar densityScale = static_cast<Scalar>(particle.getMass()) /
                                (grid.cellSize * grid.cellSize * grid.cellSize);
    for (int iz = 0; iz < zCount; ++iz) {
        for (int iy = 0; iy < yCount; ++iy) {
            for (int ix = 0; ix < xCount; ++ix) {
                density[gridIndex(x[ix], y[iy], z[iz], grid.size)] +=
                    densityScale * wx[ix] * wy[iy] * wz[iz];
            }
        }
    }
}

template <typename Scalar>
void buildFftFields(const Grid<Scalar>& grid, Scalar shortRangeScale, Scalar poissonCoefficient,
                    std::string_view assignment, CpuTreePmWorkspaceT<Scalar>& workspace)
{
    const std::size_t cellCount = workspace.density.size();
    std::vector<std::complex<Scalar>>& spectrum = workspace.spectrum;
    spectrum.resize(cellCount);
    for (std::size_t i = 0; i < cellCount; ++i) {
        spectrum[i] = std::complex<Scalar>(workspace.density[i], static_cast<Scalar>(0.0));
    }
    fft3d<Scalar>(spectrum, grid.size, false);

    const Scalar waveScale = kTwoPi<Scalar> /
                             (static_cast<Scalar>(grid.size) * grid.cellSize);
#pragma omp parallel for schedule(static)
    for (int z = 0; z < grid.size; ++z) {
        const int signedZ = z <= grid.size / 2 ? z : z - grid.size;
        for (int y = 0; y < grid.size; ++y) {
            const int signedY = y <= grid.size / 2 ? y : y - grid.size;
            for (int x = 0; x < grid.size; ++x) {
                const int signedX = x <= grid.size / 2 ? x : x - grid.size;
                const Scalar kx = static_cast<Scalar>(signedX) * waveScale;
                const Scalar ky = static_cast<Scalar>(signedY) * waveScale;
                const Scalar kz = static_cast<Scalar>(signedZ) * waveScale;
                const Scalar kSquared = kx * kx + ky * ky + kz * kz;
                const std::size_t index = static_cast<std::size_t>(gridIndex(x, y, z, grid.size));
                 if (kSquared <= static_cast<Scalar>(1.0e-12)) {
                    spectrum[index] = std::complex<Scalar>();
                    continue;
                }
                const Scalar green = poissonCoefficient *
                                     std::exp(-kSquared * shortRangeScale * shortRangeScale);
                const Scalar window = std::max(assignmentWindow(static_cast<Scalar>(0.5) * kx *
                                                                      grid.cellSize, assignment) *
                                                    assignmentWindow(static_cast<Scalar>(0.5) * ky *
                                                                      grid.cellSize, assignment) *
                                                    assignmentWindow(static_cast<Scalar>(0.5) * kz *
                                                                      grid.cellSize, assignment),
                                                static_cast<Scalar>(0.08));
                const Scalar scale = green / (kSquared * window);
                spectrum[index] *= scale;
            }
        }
    }
    fft3d<Scalar>(spectrum, grid.size, true);
    workspace.fieldX.resize(cellCount);
    workspace.fieldY.resize(cellCount);
    workspace.fieldZ.resize(cellCount);
    const Scalar inverseCellSize = static_cast<Scalar>(1.0) / grid.cellSize;
#pragma omp parallel for schedule(static)
    for (int z = 0; z < grid.size; ++z) {
        for (int y = 0; y < grid.size; ++y) {
            for (int x = 0; x < grid.size; ++x) {
                const int previousX = grid.periodic ? wrapGridIndex(x - 1, grid.size) : std::max(x - 1, 0);
                const int nextX = grid.periodic ? wrapGridIndex(x + 1, grid.size) : std::min(x + 1, grid.size - 1);
                const int previousY = grid.periodic ? wrapGridIndex(y - 1, grid.size) : std::max(y - 1, 0);
                const int nextY = grid.periodic ? wrapGridIndex(y + 1, grid.size) : std::min(y + 1, grid.size - 1);
                const int previousZ = grid.periodic ? wrapGridIndex(z - 1, grid.size) : std::max(z - 1, 0);
                const int nextZ = grid.periodic ? wrapGridIndex(z + 1, grid.size) : std::min(z + 1, grid.size - 1);
                const Scalar xGradient = (spectrum[static_cast<std::size_t>(gridIndex(
                                              nextX, y, z, grid.size))].real() -
                                          spectrum[static_cast<std::size_t>(gridIndex(
                                              previousX, y, z, grid.size))].real()) *
                                         static_cast<Scalar>(0.5) * inverseCellSize;
                const Scalar yGradient = (spectrum[static_cast<std::size_t>(gridIndex(
                                              x, nextY, z, grid.size))].real() -
                                          spectrum[static_cast<std::size_t>(gridIndex(
                                              x, previousY, z, grid.size))].real()) *
                                         static_cast<Scalar>(0.5) * inverseCellSize;
                const Scalar zGradient = (spectrum[static_cast<std::size_t>(gridIndex(
                                              x, y, nextZ, grid.size))].real() -
                                          spectrum[static_cast<std::size_t>(gridIndex(
                                              x, y, previousZ, grid.size))].real()) *
                                         static_cast<Scalar>(0.5) * inverseCellSize;
                const std::size_t index = static_cast<std::size_t>(gridIndex(x, y, z, grid.size));
                workspace.fieldX[index] = xGradient;
                workspace.fieldY[index] = yGradient;
                workspace.fieldZ[index] = zGradient;
            }
        }
    }
}

template <typename Scalar>
Vector3 sourceAcceleration(Vector3 self, const Particle& source, const ForceLawPolicy& policy)
{
    const Vector3 delta = source.getPosition() - self;
    const Scalar distance2 = static_cast<Scalar>(delta.x) * delta.x +
                             static_cast<Scalar>(delta.y) * delta.y +
                             static_cast<Scalar>(delta.z) * delta.z +
                             static_cast<Scalar>(policy.softening) * policy.softening;
    if (distance2 <= static_cast<Scalar>(policy.minDistance2)) {
        return Vector3();
    }
    const Scalar inverseDistance = static_cast<Scalar>(1.0) / std::sqrt(distance2);
    Scalar shortRangeWeight = static_cast<Scalar>(1.0);
    if (policy.treePmShortRangeScale > 0.0f) {
        const Scalar distance = static_cast<Scalar>(1.0) / inverseDistance;
        const Scalar splitScale = static_cast<Scalar>(policy.treePmShortRangeScale);
        const Scalar argument = static_cast<Scalar>(0.5) * distance / splitScale;
        shortRangeWeight = std::erfc(argument) +
                           distance / (splitScale * std::sqrt(static_cast<Scalar>(3.14159265358979323846))) *
                               std::exp(-argument * argument);
    }
    const Scalar scale = static_cast<Scalar>(source.getMass()) * inverseDistance *
                         inverseDistance * inverseDistance * shortRangeWeight;
    return Vector3(static_cast<float>(static_cast<Scalar>(delta.x) * scale),
                   static_cast<float>(static_cast<Scalar>(delta.y) * scale),
                   static_cast<float>(static_cast<Scalar>(delta.z) * scale));
}
template <typename Scalar>
bool computeCpuTreePmForcesTyped(const std::vector<Particle>& particles,
                                 const ForceLawPolicy& forceLaw,
                                 const CpuTreePmParameters& parameters,
                                 CpuTreePmWorkspaceT<Scalar>& workspace,
                                 Octree& shortRangeTree,
                                 OctreeOpeningCriterion openingCriterion,
                                 std::vector<Vector3>& forces,
                                 const std::vector<int>* activeIndices)
{
    if (particles.empty()) {
        return false;
    }
    Grid<Scalar> grid;
    const bool correctionEnabled = !parameters.periodic && parameters.model != "pm_only" &&
                                   (parameters.localGrid || parameters.model == "tree" ||
                                    parameters.model == "hybrid" || parameters.model == "auto");
    const bool treeCorrection = !parameters.periodic &&
                                (parameters.model == "tree" || parameters.model == "hybrid");
    const int maxNeighbors = std::clamp(parameters.maxLocalNeighbors, 0, 256);
    const bool rebuildField = activeIndices == nullptr || !workspace.fieldValid;
    if (rebuildField) {
        const int requestedGridSize = std::clamp(parameters.gridSize, 32, 128);
        Scalar minX = static_cast<Scalar>(particles.front().getPosition().x);
        Scalar minY = static_cast<Scalar>(particles.front().getPosition().y);
        Scalar minZ = static_cast<Scalar>(particles.front().getPosition().z);
        Scalar maxX = minX;
        Scalar maxY = minY;
        Scalar maxZ = minZ;
        for (const Particle& particle : particles) {
            const Vector3 position = particle.getPosition();
            minX = std::min(minX, static_cast<Scalar>(position.x));
            minY = std::min(minY, static_cast<Scalar>(position.y));
            minZ = std::min(minZ, static_cast<Scalar>(position.z));
            maxX = std::max(maxX, static_cast<Scalar>(position.x));
            maxY = std::max(maxY, static_cast<Scalar>(position.y));
            maxZ = std::max(maxZ, static_cast<Scalar>(position.z));
        }
        const Scalar extent = std::max({maxX - minX, maxY - minY, maxZ - minZ,
                                        static_cast<Scalar>(forceLaw.softening)});
        grid.periodic = parameters.periodic;
        grid.size = parameters.periodic ? requestedGridSize : requestedGridSize * 2;
        const Scalar periodicLength = std::max(static_cast<Scalar>(parameters.boxLength),
                                               static_cast<Scalar>(1.0e-6));
        grid.cellSize = parameters.periodic
                            ? periodicLength / static_cast<Scalar>(grid.size)
                            : std::max(static_cast<Scalar>(0.25),
                                       extent / static_cast<Scalar>(requestedGridSize - 2));
        grid.inverseCellSize = static_cast<Scalar>(1.0) / grid.cellSize;
        const Scalar halfExtent = static_cast<Scalar>(0.5) * static_cast<Scalar>(grid.size) *
                                  grid.cellSize;
        grid.originX = parameters.periodic ? static_cast<Scalar>(0.0)
                                            : static_cast<Scalar>(0.5) * (minX + maxX) - halfExtent;
        grid.originY = parameters.periodic ? static_cast<Scalar>(0.0)
                                            : static_cast<Scalar>(0.5) * (minY + maxY) - halfExtent;
        grid.originZ = parameters.periodic ? static_cast<Scalar>(0.0)
                                            : static_cast<Scalar>(0.5) * (minZ + maxZ) - halfExtent;

        const std::size_t cellCount = static_cast<std::size_t>(grid.size) * grid.size * grid.size;
        workspace.gridSize = grid.size;
        workspace.originX = grid.originX;
        workspace.originY = grid.originY;
        workspace.originZ = grid.originZ;
        workspace.cellSize = grid.cellSize;
        const Scalar cutoff = std::clamp(static_cast<Scalar>(parameters.cutoffFactor),
                                         static_cast<Scalar>(1.0), static_cast<Scalar>(2.0)) *
                              grid.cellSize;
        workspace.shortRangeScale = parameters.periodic ? static_cast<Scalar>(0.0)
                                                         : cutoff / static_cast<Scalar>(4.5);
        workspace.correctionEnabled = correctionEnabled;
        workspace.treeCorrection = treeCorrection;
        workspace.maxNeighbors = maxNeighbors;
        workspace.cutoffSquared = cutoff * cutoff;
        workspace.cellRadius = parameters.cutoffFactor > 1.0f ? 2 : 1;
        workspace.density.assign(cellCount, static_cast<Scalar>(0.0));
        // TreePM represents the full mass distribution; sampling sources would
        // solve a different gravitational system.
        const std::size_t limit = particles.size();
        for (std::size_t i = 0; i < limit; ++i) {
            depositParticle(particles[i], grid, workspace.density, parameters.assignment);
        }
        if (parameters.densityContrast) {
            const Scalar boxVolume = periodicLength * periodicLength * periodicLength;
            Scalar totalMass = static_cast<Scalar>(0.0);
            for (const Particle& particle : particles) {
                totalMass += static_cast<Scalar>(particle.getMass());
            }
            const Scalar meanDensity = totalMass / boxVolume;
            if (meanDensity <= static_cast<Scalar>(0.0)) {
                return false;
            }
            for (Scalar& density : workspace.density) {
                density = density / meanDensity - static_cast<Scalar>(1.0);
            }
        }
        buildFftFields<Scalar>(grid, workspace.shortRangeScale,
                               static_cast<Scalar>(parameters.poissonCoefficient),
                               parameters.assignment, workspace);
        workspace.fieldValid = true;
    }
    else {
        if (workspace.gridSize <= 0 || workspace.fieldX.empty() || workspace.fieldY.empty() ||
            workspace.fieldZ.empty()) {
            return false;
        }
        grid.size = workspace.gridSize;
        grid.originX = workspace.originX;
        grid.originY = workspace.originY;
        grid.originZ = workspace.originZ;
        grid.cellSize = workspace.cellSize;
        grid.inverseCellSize = static_cast<Scalar>(1.0) / grid.cellSize;
        grid.periodic = parameters.periodic;
    }

    const std::size_t cellCount = static_cast<std::size_t>(grid.size) * grid.size * grid.size;
    const Scalar cutoffSquared = workspace.cutoffSquared;
    ForceLawPolicy shortRangeLaw = forceLaw;
    shortRangeLaw.treePmShortRangeScale = static_cast<float>(workspace.shortRangeScale);
    if (forces.size() != particles.size()) {
        forces.assign(particles.size(), Vector3());
    }
    std::vector<std::pair<int, int>>& sortedCells = workspace.sortedCells;
    std::vector<int>& cellStart = workspace.cellStart;
    std::vector<int>& cellEnd = workspace.cellEnd;
    if (rebuildField) {
        sortedCells.clear();
        cellStart.clear();
        cellEnd.clear();
    }
    if (rebuildField && correctionEnabled && !treeCorrection && maxNeighbors > 0) {
        sortedCells.reserve(particles.size());
        for (int i = 0; i < static_cast<int>(particles.size()); ++i) {
            const Vector3 position = particles[static_cast<std::size_t>(i)].getPosition();
            sortedCells.emplace_back(cellHash<Scalar>(position, grid), i);
        }
        std::sort(sortedCells.begin(), sortedCells.end());
        cellStart.assign(cellCount, -1);
        cellEnd.assign(cellCount, -1);
        for (int i = 0; i < static_cast<int>(sortedCells.size()); ++i) {
            const int cell = sortedCells[static_cast<std::size_t>(i)].first;
            if (i == 0 || sortedCells[static_cast<std::size_t>(i - 1)].first != cell) {
                cellStart[static_cast<std::size_t>(cell)] = i;
            }
            if (i + 1 == static_cast<int>(sortedCells.size()) ||
                sortedCells[static_cast<std::size_t>(i + 1)].first != cell) {
                cellEnd[static_cast<std::size_t>(cell)] = i + 1;
            }
        }
    }
    if (rebuildField && treeCorrection) {
        shortRangeTree.build(particles);
    }

#pragma omp parallel for schedule(static)
    for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(
                                      activeIndices == nullptr ? particles.size() : activeIndices->size());
         ++i) {
        const std::size_t index = activeIndices == nullptr
                                      ? static_cast<std::size_t>(i)
                                      : static_cast<std::size_t>((*activeIndices)[static_cast<std::size_t>(i)]);
        const Vector3 position = particles[index].getPosition();
        forces[index] = Vector3(
            static_cast<float>(sample(workspace.fieldX, grid, position, parameters.assignment)),
            static_cast<float>(sample(workspace.fieldY, grid, position, parameters.assignment)),
            static_cast<float>(sample(workspace.fieldZ, grid, position, parameters.assignment)));
        if (treeCorrection) {
            forces[index] += shortRangeTree.computeForceOn(
                particles[index], index, shortRangeLaw, openingCriterion,
                static_cast<float>(cutoffSquared));
            continue;
        }
        if (!correctionEnabled || maxNeighbors <= 0) {
            continue;
        }
        const int centerX = coordinate(static_cast<Scalar>(position.x), grid.originX, grid);
        const int centerY = coordinate(static_cast<Scalar>(position.y), grid.originY, grid);
        const int centerZ = coordinate(static_cast<Scalar>(position.z), grid.originZ, grid);
        const int cellRadius = parameters.cutoffFactor > 1.0f ? 2 : 1;
        const int maxExamined = std::max(maxNeighbors * 4, maxNeighbors);
        int accepted = 0;
        int examined = 0;
        for (int shell = 0; shell <= 2 && accepted < maxNeighbors && examined < maxExamined;
             ++shell) {
            if (shell > cellRadius) {
                break;
            }
            for (int dz = -2; dz <= 2 && accepted < maxNeighbors && examined < maxExamined; ++dz) {
                if (std::abs(dz) > shell) {
                    continue;
                }
                const int z = centerZ + dz;
                if (z < 0 || z >= grid.size) {
                    continue;
                }
                for (int dy = -2; dy <= 2 && accepted < maxNeighbors && examined < maxExamined;
                     ++dy) {
                    if (std::abs(dy) > shell) {
                        continue;
                    }
                    const int y = centerY + dy;
                    if (y < 0 || y >= grid.size) {
                        continue;
                    }
                    for (int dx = -2; dx <= 2 && accepted < maxNeighbors && examined < maxExamined;
                         ++dx) {
                        if (std::abs(dx) > shell ||
                            std::max({std::abs(dx), std::abs(dy), std::abs(dz)}) != shell) {
                            continue;
                        }
                        const int x = centerX + dx;
                        if (x < 0 || x >= grid.size) {
                            continue;
                        }
                        const int cell = gridIndex(x, y, z, grid.size);
                        const int begin = cellStart[static_cast<std::size_t>(cell)];
                        const int end = cellEnd[static_cast<std::size_t>(cell)];
                        if (begin < 0 || end <= begin) {
                            continue;
                        }
                        for (int cursor = begin;
                             cursor < end && accepted < maxNeighbors && examined < maxExamined;
                             ++cursor) {
                            const int otherIndex = sortedCells[static_cast<std::size_t>(cursor)].second;
                            if (otherIndex == static_cast<int>(index)) {
                                continue;
                            }
                            ++examined;
                            const Particle& source = particles[static_cast<std::size_t>(otherIndex)];
                            const Vector3 delta = source.getPosition() - position;
                            const float distance2 = delta.x * delta.x + delta.y * delta.y +
                                                    delta.z * delta.z;
                            if (static_cast<Scalar>(distance2) > cutoffSquared) {
                                continue;
                            }
                            forces[index] += sourceAcceleration<Scalar>(position, source,
                                                                       shortRangeLaw);
                            ++accepted;
                        }
                    }
                }
            }
        }
    }
    return true;
}

} // namespace blitzar_physics_tree_pm_cpu

bool computeCpuTreePmForces(const std::vector<Particle>& particles,
                            const ForceLawPolicy& forceLaw,
                            const CpuTreePmParameters& parameters,
                            CpuTreePmWorkspace& workspace,
                            Octree& shortRangeTree,
                            OctreeOpeningCriterion openingCriterion,
                            std::vector<Vector3>& forces)
{
    return computeCpuTreePmForcesTyped<float>(particles, forceLaw, parameters, workspace,
                                              shortRangeTree, openingCriterion, forces, nullptr);
}

bool computeCpuTreePmForcesFp64(const std::vector<Particle>& particles,
                                const ForceLawPolicy& forceLaw,
                                const CpuTreePmParameters& parameters,
                                CpuTreePmFp64Workspace& workspace,
                                Octree& shortRangeTree,
                                OctreeOpeningCriterion openingCriterion,
                                std::vector<Vector3>& forces)
{
    return computeCpuTreePmForcesTyped<double>(particles, forceLaw, parameters, workspace,
                                               shortRangeTree, openingCriterion, forces, nullptr);
}

bool computeCpuTreePmForcesSelective(const std::vector<Particle>& particles,
                                     const std::vector<int>& activeIndices,
                                     const ForceLawPolicy& forceLaw,
                                     const CpuTreePmParameters& parameters,
                                     CpuTreePmWorkspace& workspace,
                                     Octree& shortRangeTree,
                                     OctreeOpeningCriterion openingCriterion,
                                     std::vector<Vector3>& forces)
{
    return computeCpuTreePmForcesTyped<float>(particles, forceLaw, parameters, workspace,
                                              shortRangeTree, openingCriterion, forces,
                                              &activeIndices);
}

bool computeCpuTreePmForcesSelectiveFp64(const std::vector<Particle>& particles,
                                         const std::vector<int>& activeIndices,
                                         const ForceLawPolicy& forceLaw,
                                         const CpuTreePmParameters& parameters,
                                         CpuTreePmFp64Workspace& workspace,
                                         Octree& shortRangeTree,
                                         OctreeOpeningCriterion openingCriterion,
                                         std::vector<Vector3>& forces)
{
    return computeCpuTreePmForcesTyped<double>(particles, forceLaw, parameters, workspace,
                                               shortRangeTree, openingCriterion, forces,
                                               &activeIndices);
}

bool computeCpuFp64PairwiseForces(const std::vector<Particle>& particles,
                                  const ForceLawPolicy& forceLaw,
                                  std::vector<Vector3>& forces)
{
    if (particles.empty()) {
        return false;
    }
    const std::size_t count = particles.size();
    forces.assign(count, Vector3());
#pragma omp parallel for schedule(static)
    for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(count); ++i) {
        const std::size_t index = static_cast<std::size_t>(i);
        const Vector3 self = particles[index].getPosition();
        double forceX = 0.0;
        double forceY = 0.0;
        double forceZ = 0.0;
        for (std::size_t j = 0; j < count; ++j) {
            if (index == j) {
                continue;
            }
            const Vector3 delta = particles[j].getPosition() - self;
            const double distance2 = static_cast<double>(delta.x) * delta.x +
                                     static_cast<double>(delta.y) * delta.y +
                                     static_cast<double>(delta.z) * delta.z +
                                     static_cast<double>(forceLaw.softening) * forceLaw.softening;
            if (distance2 <= static_cast<double>(forceLaw.minDistance2)) {
                continue;
            }
            const double inverseDistance = 1.0 / std::sqrt(distance2);
            const double scale = static_cast<double>(particles[j].getMass()) * inverseDistance *
                                 inverseDistance * inverseDistance;
            forceX += static_cast<double>(delta.x) * scale;
            forceY += static_cast<double>(delta.y) * scale;
            forceZ += static_cast<double>(delta.z) * scale;
        }
        forces[index] = Vector3(static_cast<float>(forceX), static_cast<float>(forceY),
                                static_cast<float>(forceZ));
    }
    return true;
}
