namespace blitzar_accelerator_launch {

namespace {

__device__ void RecordError(int* error, int status) noexcept
{
    atomicMax(error, status);
}

} // namespace

} // namespace blitzar_accelerator_launch
