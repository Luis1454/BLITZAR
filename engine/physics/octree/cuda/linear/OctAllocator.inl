/*
 * @file engine/physics/octree/cuda/linear/OctAllocator.inl
 * @project BLITZAR
 * @brief GPU linear octree implementation fragment.
 */

/*
 * @file engine/physics/octree/cuda/linear/OctAllocator.inl
 * @author Luis1454
 * @project BLITZAR
 * @brief Physics and CUDA implementation for the deterministic simulation core.
 */

/*
 * Module: cuda
 * Responsibility: Build a true 8-way octree fully on GPU.
 */

#include <cfloat>
#include <chrono>
#include <cstddef>
#include <cub/cub.cuh>
#include <thrust/device_ptr.h>
#include <thrust/execution_policy.h>
#include <thrust/iterator/constant_iterator.h>
#include <thrust/iterator/zip_iterator.h>
#include <thrust/pair.h>
#include <thrust/reduce.h>
#include <thrust/scan.h>
#include <thrust/sort.h>
#include <thrust/transform_reduce.h>
#include <thrust/tuple.h>

/*
 * @brief Defines the thrust pool allocator type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct ThrustPoolAllocator {
    typedef char value_type;
    struct CachedBlock {
        char* ptr = nullptr;
        std::size_t bytes = 0u;
        bool inUse = false;
    };

    static constexpr int kMaxCachedBlocks = 64;
    static CachedBlock _cachedBlocks[kMaxCachedBlocks];

    /*
     * @brief Documents the allocate operation contract.
     * @param numBytes Input value used by this contract.
     * @return char* value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    char* allocate(std::ptrdiff_t numBytes)
    {
        const std::size_t bytes = static_cast<std::size_t>(std::max<std::ptrdiff_t>(0, numBytes));
        if (bytes == 0u) {
            return nullptr;
        }
        int firstEmptySlot = -1;
        for (int index = 0; index < kMaxCachedBlocks; ++index) {
            CachedBlock& block = _cachedBlocks[index];
            if (block.ptr == nullptr) {
                if (firstEmptySlot < 0) {
                    firstEmptySlot = index;
                }
                continue;
            }
            if (block.inUse || block.bytes < bytes) {
                continue;
            }
            cudaPointerAttributes attributes{};
            if (cudaPointerGetAttributes(&attributes, block.ptr) != cudaSuccess) {
                cudaGetLastError();
                block.ptr = nullptr;
                block.bytes = 0u;
                if (firstEmptySlot < 0) {
                    firstEmptySlot = index;
                }
                continue;
            }
            block.inUse = true;
            return block.ptr;
        }
        if (firstEmptySlot >= 0) {
            char* ptr = static_cast<char*>(bltzr_x::MemoryPool::allocate(bytes));
            if (ptr != nullptr) {
                _cachedBlocks[firstEmptySlot] = CachedBlock{ptr, bytes, true};
            }
            return ptr;
        }
        return static_cast<char*>(bltzr_x::MemoryPool::allocate(bytes));
    }

    /*
     * @brief Documents the deallocate operation contract.
     * @param ptr Input value used by this contract.
     * @param size_t Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void deallocate(char* ptr, std::size_t)
    {
        if (ptr == nullptr) {
            return;
        }
        for (int index = 0; index < kMaxCachedBlocks; ++index) {
            CachedBlock& block = _cachedBlocks[index];
            if (block.ptr == ptr) {
                block.inUse = false;
                return;
            }
        }
        bltzr_x::MemoryPool::deallocate(ptr);
    }
};

ThrustPoolAllocator::CachedBlock
    ThrustPoolAllocator::_cachedBlocks[ThrustPoolAllocator::kMaxCachedBlocks]{};

/*
 * @brief Defines the octree aabb type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct OctreeAabb {
    float minX;
    float minY;
    float minZ;
    float maxX;
    float maxY;
    float maxZ;
};

/*
 * @brief Defines the octree aabb from tuple type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct OctreeAabbFromTuple {
    /*
     * @brief Documents the operator operation contract.
     * @param value Input value used by this contract.
     * @return OctreeAabb value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    __host__ __device__ OctreeAabb operator()(const thrust::tuple<float, float, float>& value) const
    {
        const float x = thrust::get<0>(value);
        const float y = thrust::get<1>(value);
        const float z = thrust::get<2>(value);
        OctreeAabb out{};
        out.minX = x;
        out.minY = y;
        out.minZ = z;
        out.maxX = x;
        out.maxY = y;
        out.maxZ = z;
        return out;
    }
};

/*
 * @brief Defines the octree aabb merge type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct OctreeAabbMerge {
    /*
     * @brief Documents the operator operation contract.
     * @param rhs Input value used by this contract.
     * @return OctreeAabb value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    __host__ __device__ OctreeAabb operator()(const OctreeAabb& lhs, const OctreeAabb& rhs) const
    {
        OctreeAabb out{};
        out.minX = fminf(lhs.minX, rhs.minX);
        out.minY = fminf(lhs.minY, rhs.minY);
        out.minZ = fminf(lhs.minZ, rhs.minZ);
        out.maxX = fmaxf(lhs.maxX, rhs.maxX);
        out.maxY = fmaxf(lhs.maxY, rhs.maxY);
        out.maxZ = fmaxf(lhs.maxZ, rhs.maxZ);
        return out;
    }
};
