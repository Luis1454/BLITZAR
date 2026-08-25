__device__ bool IsLeaf(const GpuCell& cell) noexcept
{
    for (unsigned int child = 0; child < 8; ++child) {
        if (cell.children[child] != InvalidCell) {
            return false;
        }
    }

    return true;
}

__device__ bool Contains(
    const GpuCell& cell, double position_x, double position_y, double position_z) noexcept
{
    return fabs(position_x - cell.center_x) <= cell.half_extent &&
           fabs(position_y - cell.center_y) <= cell.half_extent &&
           fabs(position_z - cell.center_z) <= cell.half_extent;
}

__device__ int ValidateCell(
    const BarnesHutDeviceRequest& request, const GpuCell& cell) noexcept
{
    if (!isfinite(cell.mass) || cell.mass < 0.0 || cell.begin > request.source_count ||
        cell.count > request.source_count - cell.begin) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    return BLITZAR_STATUS_OK;
}
