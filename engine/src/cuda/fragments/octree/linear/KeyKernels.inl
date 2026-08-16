/*
 * @file engine/src/cuda/fragments/octree/linear/KeyKernels.inl
 * @project BLITZAR
 * @brief GPU linear octree implementation fragment.
 */

/*
 * @brief Documents the build morton codes kernel operation contract.
 * @param state Input value used by this contract.
 * @param numParticles Input value used by this contract.
 * @param minX Input value used by this contract.
 * @param minY Input value used by this contract.
 * @param minZ Input value used by this contract.
 * @param maxX Input value used by this contract.
 * @param maxY Input value used by this contract.
 * @param maxZ Input value used by this contract.
 * @param mortonKeys Input value used by this contract.
 * @param particleIndices Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void buildMortonCodesKernel(ParticleSoAView state, int numParticles, float minX,
                                       float minY, float minZ, float maxX, float maxY, float maxZ,
                                       unsigned long long* mortonKeys, int* particleIndices)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }

    const Vector3 pos = getSoAPosition(state, i);
    const float invX = 1.0f / fmaxf(maxX - minX, 1.0e-12f);
    const float invY = 1.0f / fmaxf(maxY - minY, 1.0e-12f);
    const float invZ = 1.0f / fmaxf(maxZ - minZ, 1.0e-12f);

    const float nx = fminf(fmaxf((pos.x - minX) * invX, 0.0f), 1.0f);
    const float ny = fminf(fmaxf((pos.y - minY) * invY, 0.0f), 1.0f);
    const float nz = fminf(fmaxf((pos.z - minZ) * invZ, 0.0f), 1.0f);

    const unsigned int qx = min(2097151u, static_cast<unsigned int>(nx * 2097151.0f + 0.5f));
    const unsigned int qy = min(2097151u, static_cast<unsigned int>(ny * 2097151.0f + 0.5f));
    const unsigned int qz = min(2097151u, static_cast<unsigned int>(nz * 2097151.0f + 0.5f));

    mortonKeys[i] = mortonEncode63(qx, qy, qz);
    particleIndices[i] = i;
}

/*
 * @brief Documents the build leaf prefixes kernel operation contract.
 * @param sortedKeys Input value used by this contract.
 * @param count Input value used by this contract.
 * @param shiftBits Input value used by this contract.
 * @param outLeafPrefixes Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void buildLeafPrefixesKernel(const unsigned long long* sortedKeys, int count,
                                        int shiftBits, unsigned long long* outLeafPrefixes)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= count) {
        return;
    }
    outLeafPrefixes[i] = sortedKeys[i] >> shiftBits;
}

/*
 * @brief Documents the build parent prefixes kernel operation contract.
 * @param currentPrefixes Input value used by this contract.
 * @param count Input value used by this contract.
 * @param outParentPrefixes Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void buildParentPrefixesKernel(const unsigned long long* currentPrefixes, int count,
                                          unsigned long long* outParentPrefixes)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= count) {
        return;
    }
    outParentPrefixes[i] = currentPrefixes[i] >> 3;
}

/*
 * @brief Documents the init level indices kernel operation contract.
 * @param levelIndices Input value used by this contract.
 * @param count Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
__global__ void initLevelIndicesKernel(int* levelIndices, int count)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < count) {
        levelIndices[i] = i;
    }
}

/*
 * @brief Documents the build linear octree leaf nodes kernel operation contract.
 * @param nodes Input value used by this contract.
 * @param sortedParticleIndices Input value used by this contract.
 * @param leafStarts Input value used by this contract.
 * @param leafCounts Input value used by this contract.
 * @param state Input value used by this contract.
 * @param leafCount Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
