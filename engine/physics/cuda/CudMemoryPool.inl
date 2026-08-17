/*
 * @file engine/physics/cuda/CudMemoryPool.inl
 * @project BLITZAR
 * @brief RAII adapter for pooled device buffers.
 */

namespace bltzr_x {

template <typename T>
void MemoryPool::deallocate(blitzar_cuda_memory::DeviceBuffer<T>& buffer)
{
    buffer.reset();
}

} // namespace bltzr_x
