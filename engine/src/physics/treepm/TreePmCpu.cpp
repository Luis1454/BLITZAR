/*
 * @file engine/src/physics/treepm/TreePmCpu.cpp
 * @brief Deterministic CPU TreePM field construction and sampling.
 */

#include "physics/treepm/TreePmCpu.hpp"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <functional>
#include <numeric>
#include <omp.h>
#include <optional>
#include <string_view>
#include <utility>

namespace blitzar_physics_tree_pm_cpu {
template <typename Scalar> constexpr Scalar kTwoPi = static_cast<Scalar>(6.2831853071795864769);

template <typename Scalar> constexpr Scalar kFourPi = static_cast<Scalar>(12.566370614359172);

int gridIndex(int x, int y, int z, int gridSize)
{
    return (z * gridSize + y) * gridSize + x;
}

template <typename Scalar> Scalar modifiedBesselK1(Scalar value)
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
        const Scalar i1 = x * static_cast<Scalar>(0.5) * (static_cast<Scalar>(1.0) + y * i1Poly);
        Scalar k1Poly = static_cast<Scalar>(-0.00004686);
        k1Poly = static_cast<Scalar>(-0.00110404) + y * k1Poly;
        k1Poly = static_cast<Scalar>(-0.01919402) + y * k1Poly;
        k1Poly = static_cast<Scalar>(-0.18156897) + y * k1Poly;
        k1Poly = static_cast<Scalar>(-0.67278579) + y * k1Poly;
        k1Poly = static_cast<Scalar>(0.15443144) + y * k1Poly;
        return std::log(x * static_cast<Scalar>(0.5)) * i1 +
               (static_cast<Scalar>(1.0) / x) * (static_cast<Scalar>(1.0) + y * k1Poly);
    }
    const Scalar y = static_cast<Scalar>(2.0) / x;
    Scalar asymptotic = static_cast<Scalar>(-0.00068245);
    asymptotic = static_cast<Scalar>(0.00325614) + y * asymptotic;
    asymptotic = static_cast<Scalar>(-0.00780353) + y * asymptotic;
    asymptotic = static_cast<Scalar>(0.01504268) + y * asymptotic;
    asymptotic = static_cast<Scalar>(-0.03655620) + y * asymptotic;
    asymptotic = static_cast<Scalar>(0.23498619) + y * asymptotic;
    return std::exp(-x) * (static_cast<Scalar>(1.0) / std::sqrt(x)) *
           static_cast<Scalar>(1.25331414) * (static_cast<Scalar>(1.0) + y * asymptotic);
}

template <typename Scalar> Scalar sinc(Scalar value)
{
    return std::fabs(value) < static_cast<Scalar>(1.0e-5) ? static_cast<Scalar>(1.0)
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
                const std::size_t evenIndex =
                    static_cast<std::size_t>(start + (offset + i) * stride);
                const std::size_t oddIndex =
                    static_cast<std::size_t>(start + (offset + i + halfLength) * stride);
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
        const Scalar inverseCells =
            static_cast<Scalar>(1.0) / static_cast<Scalar>(size * size * size);
        for (std::complex<Scalar>& value : values) {
            value *= inverseCells;
        }
    }
}

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

    const Scalar waveScale = kTwoPi<Scalar> / (static_cast<Scalar>(grid.size) * grid.cellSize);
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
                const Scalar green =
                    poissonCoefficient * std::exp(-kSquared * shortRangeScale * shortRangeScale);
                const Scalar window = std::max(
                    assignmentWindow(static_cast<Scalar>(0.5) * kx * grid.cellSize, assignment) *
                        assignmentWindow(static_cast<Scalar>(0.5) * ky * grid.cellSize,
                                         assignment) *
                        assignmentWindow(static_cast<Scalar>(0.5) * kz * grid.cellSize, assignment),
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
                const int previousX =
                    grid.periodic ? wrapGridIndex(x - 1, grid.size) : std::max(x - 1, 0);
                const int nextX = grid.periodic ? wrapGridIndex(x + 1, grid.size)
                                                : std::min(x + 1, grid.size - 1);
                const int previousY =
                    grid.periodic ? wrapGridIndex(y - 1, grid.size) : std::max(y - 1, 0);
                const int nextY = grid.periodic ? wrapGridIndex(y + 1, grid.size)
                                                : std::min(y + 1, grid.size - 1);
                const int previousZ =
                    grid.periodic ? wrapGridIndex(z - 1, grid.size) : std::max(z - 1, 0);
                const int nextZ = grid.periodic ? wrapGridIndex(z + 1, grid.size)
                                                : std::min(z + 1, grid.size - 1);
                const Scalar xGradient =
                    (spectrum[static_cast<std::size_t>(gridIndex(nextX, y, z, grid.size))].real() -
                     spectrum[static_cast<std::size_t>(gridIndex(previousX, y, z, grid.size))]
                         .real()) *
                    static_cast<Scalar>(0.5) * inverseCellSize;
                const Scalar yGradient =
                    (spectrum[static_cast<std::size_t>(gridIndex(x, nextY, z, grid.size))].real() -
                     spectrum[static_cast<std::size_t>(gridIndex(x, previousY, z, grid.size))]
                         .real()) *
                    static_cast<Scalar>(0.5) * inverseCellSize;
                const Scalar zGradient =
                    (spectrum[static_cast<std::size_t>(gridIndex(x, y, nextZ, grid.size))].real() -
                     spectrum[static_cast<std::size_t>(gridIndex(x, y, previousZ, grid.size))]
                         .real()) *
                    static_cast<Scalar>(0.5) * inverseCellSize;
                const std::size_t index = static_cast<std::size_t>(gridIndex(x, y, z, grid.size));
                workspace.fieldX[index] = xGradient;
                workspace.fieldY[index] = yGradient;
                workspace.fieldZ[index] = zGradient;
            }
        }
    }
}

#include "TreePmForce.inl"
} // namespace blitzar_physics_tree_pm_cpu

bool computeCpuTreePmForces(const std::vector<Particle>& particles, const ForceLawPolicy& forceLaw,
                            const CpuTreePmParameters& parameters, CpuTreePmWorkspace& workspace,
                            Octree& shortRangeTree, OctreeOpeningCriterion openingCriterion,
                            std::vector<Vector3>& forces)
{
    return blitzar_physics_tree_pm_cpu::computeCpuTreePmForcesTyped<float>(
        particles, forceLaw, parameters, workspace, shortRangeTree, openingCriterion, forces,
        std::nullopt);
}

bool computeCpuTreePmForcesFp64(const std::vector<Particle>& particles,
                                const ForceLawPolicy& forceLaw,
                                const CpuTreePmParameters& parameters,
                                CpuTreePmFp64Workspace& workspace, Octree& shortRangeTree,
                                OctreeOpeningCriterion openingCriterion,
                                std::vector<Vector3>& forces)
{
    return blitzar_physics_tree_pm_cpu::computeCpuTreePmForcesTyped<double>(
        particles, forceLaw, parameters, workspace, shortRangeTree, openingCriterion, forces,
        std::nullopt);
}

bool computeCpuTreePmForcesSelective(const std::vector<Particle>& particles,
                                     const std::vector<int>& activeIndices,
                                     const ForceLawPolicy& forceLaw,
                                     const CpuTreePmParameters& parameters,
                                     CpuTreePmWorkspace& workspace, Octree& shortRangeTree,
                                     OctreeOpeningCriterion openingCriterion,
                                     std::vector<Vector3>& forces)
{
    return blitzar_physics_tree_pm_cpu::computeCpuTreePmForcesTyped<float>(
        particles, forceLaw, parameters, workspace, shortRangeTree, openingCriterion, forces,
        std::cref(activeIndices));
}

bool computeCpuTreePmForcesSelectiveFp64(const std::vector<Particle>& particles,
                                         const std::vector<int>& activeIndices,
                                         const ForceLawPolicy& forceLaw,
                                         const CpuTreePmParameters& parameters,
                                         CpuTreePmFp64Workspace& workspace, Octree& shortRangeTree,
                                         OctreeOpeningCriterion openingCriterion,
                                         std::vector<Vector3>& forces)
{
    return blitzar_physics_tree_pm_cpu::computeCpuTreePmForcesTyped<double>(
        particles, forceLaw, parameters, workspace, shortRangeTree, openingCriterion, forces,
        std::cref(activeIndices));
}

bool computeCpuFp64PairwiseForces(const std::vector<Particle>& particles,
                                  const ForceLawPolicy& forceLaw, std::vector<Vector3>& forces)
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
