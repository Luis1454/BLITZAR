/*
 * @file engine/physics/octree/morton/OctMortonCodes.inl
 * @project BLITZAR
 * @brief Shared Morton coding primitives for octree spatial ordering.
 */

__device__ __forceinline__ unsigned long long octreeExpandBits21(unsigned int value)
{
    value = (value | (value << 16)) & 0x030000FFu;
    value = (value | (value << 8)) & 0x0300F00Fu;
    value = (value | (value << 4)) & 0x030C30C3u;
    value = (value | (value << 2)) & 0x09249249u;
    return static_cast<unsigned long long>(value);
}

__device__ __forceinline__ unsigned long long mortonEncode63(unsigned int x,
                                                               unsigned int y,
                                                               unsigned int z)
{
    return (octreeExpandBits21(x) << 2) | (octreeExpandBits21(y) << 1) |
           octreeExpandBits21(z);
}
