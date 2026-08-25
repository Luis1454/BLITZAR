namespace blitzar_gpu_detail {

namespace {

__device__ void RecordError(int* error, int status) noexcept
{
    atomicMax(error, status);
}

} // namespace

} // namespace blitzar_gpu_detail
