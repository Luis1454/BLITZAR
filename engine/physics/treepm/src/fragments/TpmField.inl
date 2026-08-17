/*
 * @file engine/physics/treepm/src/fragments/TpmField.inl
 * @brief Fourier-space Poisson solve and real-space field construction.
 */

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
                spectrum[index] *= green / (kSquared * window);
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
