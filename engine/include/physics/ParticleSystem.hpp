/*
 * @file engine/include/physics/ParticleSystem.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Public physics interfaces and data contracts for deterministic simulation kernels.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_PHYSICS_PARTICLESYSTEM_HPP_
#define BLITZAR_ENGINE_INCLUDE_PHYSICS_PARTICLESYSTEM_HPP_

#include "physics/CudaMemoryPool.hpp"
#include "physics/ForceLawPolicy.hpp"
#include "physics/Octree.hpp"
#include "physics/ParticleSoAView.hpp"

#include <vector_types.h>

/*
 * Module: physics
 * Responsibility: Own the particle-state buffers and advance the gravitational/SPH simulation.
 */

#include <cstdint>
#include <string>
#include <vector>

/*
 * @brief Defines the alignas type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct alignas(64) GpuSystemMetrics {
    std::uint32_t sequence;
    std::uint32_t flags;
    std::uint64_t stepId;
    float simTime;
    float dt;
    std::uint32_t particleCount;
    std::uint32_t nanCount;
    std::uint32_t infCount;
    float minSpeed;
    float maxSpeed;
    float kineticEnergy;
    float potentialEnergy;
    float totalEnergy;
    std::uint64_t vramUsedBytes;
    std::uint64_t vramPeakBytes;
    std::uint32_t reserved0;
    std::uint32_t reservedAlignment;
    std::uint64_t reserved1;
    std::uint64_t reserved2;
    std::uint64_t reserved3;
    std::uint64_t reserved4;
    std::uint64_t reserved5;
    std::uint64_t reserved6;
};

/*
 * @brief Defines the gpu metrics payload type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct GpuMetricsPayload {
    std::uint32_t flags;
    std::uint64_t stepId;
    float simTime;
    float dt;
    std::uint32_t particleCount;
    std::uint32_t nanCount;
    std::uint32_t infCount;
    float minSpeed;
    float maxSpeed;
    float kineticEnergy;
    float potentialEnergy;
    float totalEnergy;
    std::uint64_t vramUsedBytes;
    std::uint64_t vramPeakBytes;
};

/*
 * @brief Defines the gpu metrics flags type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
enum GpuMetricsFlags : std::uint32_t {
    kGpuMetricsValid = 1u << 0,
    kGpuMetricsEstimated = 1u << 1
};

/*
 * @brief Defines the particle system type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class ParticleSystem {
public:
    /*
     * @brief Defines the solver mode type contract.
     * @param None This contract does not take explicit parameters.
     * @return Not applicable; this block documents a type contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    enum class SolverMode {
        PairwiseCuda,
        OctreeCpu,
        OctreeGpu
    };
    /*
     * @brief Defines the integrator mode type contract.
     * @param None This contract does not take explicit parameters.
     * @return Not applicable; this block documents a type contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    enum class IntegratorMode {
        Euler,
        Rk4,
        Leapfrog
    };

    ParticleSystem(int numParticles, bool bootstrapInitialState = true);
    /*
     * @brief Documents the particle system operation contract.
     * @param initialParticles Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    explicit ParticleSystem(std::vector<Particle> initialParticles);
    /*
     * @brief Documents the ~particle system operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    ~ParticleSystem();

    /*
     * @brief Documents the update operation contract.
     * @param deltaTime Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool update(float deltaTime);
    /*
     * @brief Documents the set use octree operation contract.
     * @param enabled Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setUseOctree(bool enabled);
    /*
     * @brief Documents the uses octree operation contract.
     * @param None This contract does not take explicit parameters.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool usesOctree() const;
    /*
     * @brief Documents the set octree theta operation contract.
     * @param theta Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setOctreeTheta(float theta);
    /*
     * @brief Documents the set octree opening criterion operation contract.
     * @param criterion Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setOctreeOpeningCriterion(OctreeOpeningCriterion criterion);
    /*
     * @brief Documents the set octree softening operation contract.
     * @param softening Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setOctreeSoftening(float softening);
    /*
     * @brief Documents the set sph enabled operation contract.
     * @param enabled Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setSphEnabled(bool enabled);
    /*
     * @brief Documents the is sph enabled operation contract.
     * @param None This contract does not take explicit parameters.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool isSphEnabled() const;
    /*
     * @brief Documents the set sph parameters operation contract.
     * @param smoothingLength Input value used by this contract.
     * @param restDensity Input value used by this contract.
     * @param gasConstant Input value used by this contract.
     * @param viscosity Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setSphParameters(float smoothingLength, float restDensity, float gasConstant,
                          float viscosity);
    /*
     * @brief Documents the set physics stability constants operation contract.
     * @param maxAcceleration Input value used by this contract.
     * @param minSoftening Input value used by this contract.
     * @param minDistance2 Input value used by this contract.
     * @param minTheta Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setPhysicsStabilityConstants(float maxAcceleration, float minSoftening, float minDistance2,
                                      float minTheta);
    /*
     * @brief Documents the set sph caps operation contract.
     * @param maxAcceleration Input value used by this contract.
     * @param maxSpeed Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setSphCaps(float maxAcceleration, float maxSpeed);
    /*
     * @brief Documents the set thermal parameters operation contract.
     * @param ambientTemperature Input value used by this contract.
     * @param specificHeat Input value used by this contract.
     * @param heatingCoeff Input value used by this contract.
     * @param radiationCoeff Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setThermalParameters(float ambientTemperature, float specificHeat, float heatingCoeff,
                              float radiationCoeff);
    /*
     * @brief Documents the get cumulative radiated energy operation contract.
     * @param None This contract does not take explicit parameters.
     * @return float value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    float getCumulativeRadiatedEnergy() const;
    /*
     * @brief Documents the get thermal specific heat operation contract.
     * @param None This contract does not take explicit parameters.
     * @return float value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    float getThermalSpecificHeat() const;
    /*
     * @brief Documents the set solver mode operation contract.
     * @param mode Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setSolverMode(SolverMode mode);
    /*
     * @brief Documents the get solver mode operation contract.
     * @param None This contract does not take explicit parameters.
     * @return SolverMode value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    SolverMode getSolverMode() const;
    /*
     * @brief Documents the set integrator mode operation contract.
     * @param mode Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setIntegratorMode(IntegratorMode mode);
    /*
     * @brief Documents the get integrator mode operation contract.
     * @param None This contract does not take explicit parameters.
     * @return IntegratorMode value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    IntegratorMode getIntegratorMode() const;
    /*
     * @brief Documents the sync device state operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void syncDeviceState();
    /*
     * @brief Documents the sync host state operation contract.
     * @param None This contract does not take explicit parameters.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool syncHostState();
    /*
     * @brief Documents the compute energy estimate gpu operation contract.
     * @param sampleLimit Input value used by this contract.
     * @param softening Input value used by this contract.
     * @param minDistance2 Input value used by this contract.
     * @param specificHeat Input value used by this contract.
     * @param kinetic Input value used by this contract.
     * @param potential Input value used by this contract.
     * @param thermal Input value used by this contract.
     * @param estimated Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool computeEnergyEstimateGpu(std::size_t sampleLimit, float softening, float minDistance2,
                                  float specificHeat, float& kinetic, float& potential,
                                  float& thermal, bool& estimated);

    /*
     * @brief Documents the get particles operation contract.
     * @param None This contract does not take explicit parameters.
     * @return const std::vector<Particle>& value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    const std::vector<Particle>& getParticles() const;
    /*
     * @brief Documents the set particles operation contract.
     * @param particles Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool setParticles(std::vector<Particle> particles);

    ParticleSystem(const ParticleSystem&) = delete;
    ParticleSystem& operator=(const ParticleSystem&) = delete;
    ParticleSystem(ParticleSystem&&) = delete;
    ParticleSystem& operator=(ParticleSystem&&) = delete;

    /*
     * @brief Documents the get so aview operation contract.
     * @param next Input value used by this contract.
     * @return ParticleSoAView value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    ParticleSoAView getSoAView(bool next = false) const;
    /*
     * @brief Documents the get mapped gpu metrics operation contract.
     * @param None This contract does not take explicit parameters.
     * @return const GpuSystemMetrics* value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    const GpuSystemMetrics* getMappedGpuMetrics() const;

private:
    /*
     * @brief Documents the initialize runtime state operation contract.
     * @param particleCapacity Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void initializeRuntimeState(std::size_t particleCapacity);
    /*
     * @brief Documents the build bootstrap state operation contract.
     * @param particleCount Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void buildBootstrapState(int particleCount);
    /*
     * @brief Documents the allocate particle buffers operation contract.
     * @param particleCapacity Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool allocateParticleBuffers(std::size_t particleCapacity);
    /*
     * @brief Documents the seed device state operation contract.
     * @param None This contract does not take explicit parameters.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool seedDeviceState();
    /*
     * @brief Documents the release particle buffers operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void releaseParticleBuffers();
    /*
     * @brief Documents the apply thermal model operation contract.
     * @param deltaTime Input value used by this contract.
     * @return float value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    float applyThermalModel(float deltaTime);
    /*
     * @brief Documents the build sph grid operation contract.
     * @param numParticles Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool buildSphGrid(int numParticles);
    /*
     * @brief Documents the release rk4 buffers operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void releaseRk4Buffers();
    /*
     * @brief Documents the release sph buffers operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void releaseSphBuffers();
    /*
     * @brief Documents the release sph grid buffers operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void releaseSphGridBuffers();
    /*
     * @brief Documents the allocate rk4 buffers operation contract.
     * @param numParticles Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool allocateRk4Buffers(int numParticles);
    /*
     * @brief Documents the allocate sph buffers operation contract.
     * @param numParticles Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool allocateSphBuffers(int numParticles);
    /*
     * @brief Documents the allocate sph grid buffers operation contract.
     * @param numParticles Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool allocateSphGridBuffers(int numParticles);
    /*
     * @brief Documents the ensure linear octree scratch capacity operation contract.
     * @param numParticles Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool ensureLinearOctreeScratchCapacity(int numParticles);
    /*
     * @brief Documents the ensure energy scratch capacity operation contract.
     * @param numParticles Input value used by this contract.
     * @param sampleCount Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool ensureEnergyScratchCapacity(int numParticles, int sampleCount);
    /*
     * @brief Documents the build linear octree gpu operation contract.
     * @param currentView Input value used by this contract.
     * @param numParticles Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool buildLinearOctreeGpu(ParticleSoAView currentView, int numParticles);
    /*
     * @brief Documents the allocate mapped metrics operation contract.
     * @param None This contract does not take explicit parameters.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool allocateMappedMetrics();
    /*
     * @brief Documents the release mapped metrics operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void releaseMappedMetrics();
    /*
     * @brief Documents the publish mapped metrics operation contract.
     * @param deltaTime Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void publishMappedMetrics(float deltaTime);
    /*
     * @brief Documents the estimate memory usage operation contract.
     * @param particleCount Input value used by this contract.
     * @param sphEnabled Input value used by this contract.
     * @param solverMode Input value used by this contract.
     * @param integratorMode Input value used by this contract.
     * @param energySampleLimit Input value used by this contract.
     * @param octreeLeafCapacity Input value used by this contract.
     * @param baseAndIntegratorBytes Input value used by this contract.
     * @param sphBytes Input value used by this contract.
     * @param octreeBytes Input value used by this contract.
     * @return std::size_t value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    std::size_t estimateMemoryUsage(std::size_t particleCount, bool sphEnabled,
                                    SolverMode solverMode, IntegratorMode integratorMode,
                                    std::size_t energySampleLimit, int octreeLeafCapacity,
                                    std::size_t* baseAndIntegratorBytes, std::size_t* sphBytes,
                                    std::size_t* octreeBytes) const;
    /*
     * @brief Documents the format memory breakdown operation contract.
     * @param baseAndIntegratorBytes Input value used by this contract.
     * @param sphBytes Input value used by this contract.
     * @param octreeBytes Input value used by this contract.
     * @param totalBytes Input value used by this contract.
     * @param budgetBytes Input value used by this contract.
     * @return std::string value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    static std::string formatMemoryBreakdown(std::size_t baseAndIntegratorBytes,
                                             std::size_t sphBytes, std::size_t octreeBytes,
                                             std::size_t totalBytes, std::size_t budgetBytes);

    std::vector<Particle> _particles;
    SolverMode _solverMode;
    IntegratorMode _integratorMode;
    float _octreeTheta;
    OctreeOpeningCriterion _octreeOpeningCriterion;
    float _octreeSoftening;
    bool _sphEnabled;
    float _sphSmoothingLength;
    float _sphRestDensity;
    float _sphGasConstant;
    float _sphViscosity;
    float _physicsMaxAcceleration;
    float _physicsMinSoftening;
    float _physicsMinDistance2;
    float _physicsMinTheta;
    float _sphMaxAcceleration;
    float _sphMaxSpeed;
    float _thermalAmbientTemperature;
    float _thermalSpecificHeat;
    float _thermalHeatingCoeff;
    float _thermalRadiationCoeff;
    float _cumulativeRadiatedEnergy;
    Octree _octree;
    std::vector<Vector3> _octreeForces;
    std::vector<GpuOctreeNode> _octreeGpuNodes;
    std::vector<int> _octreeGpuLeafIndices;
    std::size_t _deviceParticleCapacity;
    bool _cudaRuntimeAvailable;
    bool _hostStateDirty;
    int _sphGridSize;
    int _sphGridTotalCells;

    // Device buffers (SoA layout)
    float* d_soaPosX;
    float* d_soaPosY;
    float* d_soaPosZ;
    float* d_soaVelX;
    float* d_soaVelY;
    float* d_soaVelZ;
    float* d_soaPressX;
    float* d_soaPressY;
    float* d_soaPressZ;
    float* d_soaMass;
    float* d_soaTemp;
    float* d_soaDens;

    // Double buffering for update
    float* d_soaNextPosX;
    float* d_soaNextPosY;
    float* d_soaNextPosZ;
    float* d_soaNextVelX;
    float* d_soaNextVelY;
    float* d_soaNextVelZ;

    // Stage buffer for RK4 or thermal sync
    Particle* d_stage;

    Vector3* d_k1x;
    Vector3* d_k2x;
    Vector3* d_k3x;
    Vector3* d_k4x;
    Vector3* d_k1v;
    Vector3* d_k2v;
    Vector3* d_k3v;
    Vector3* d_k4v;
    float3* d_vHalf;
    bool _leapfrogPrimed;
    float* d_sphDensity;
    float* d_sphPressure;
    int* d_sphCellHash;
    int* d_sphSortedIndex;
    int* d_sphCellStart;
    int* d_sphCellEnd;
    GpuOctreeNode* g_dOctreeNodes;
    int* g_dOctreeLeafIndices;
    unsigned long long* d_octreeMortonKeys;
    unsigned long long* d_octreePrefixesA;
    unsigned long long* d_octreePrefixesB;
    int* d_octreeLevelIndicesA;
    int* d_octreeLevelIndicesB;
    int* d_octreeParentCounts;
    int* d_octreeParentOffsets;
    GpuOctreeNodeHotData* d_octreeNodeHot;
    GpuOctreeNodeNavData* d_octreeNodeNav;
    int* d_octreeFirstChild;
    int* d_octreeLeafStarts;
    int* d_octreeLeafCounts;
    float* d_energyKineticBlocks;
    float* d_energyThermalBlocks;
    double* d_energyPotentialPartials;

    // Host shadows for SPH
    std::vector<int> _hostCellHash;
    std::vector<int> _hostSortedIndex;

    // Octree capacity tracking
    std::size_t g_dOctreeNodeCapacity;
    std::size_t g_dOctreeLeafCapacity;
    std::size_t d_octreeMortonCapacity;
    std::size_t d_octreePrefixCapacity;
    std::size_t d_octreeLevelCapacity;
    std::size_t d_energyBlockCapacity;
    std::size_t d_energySampleCapacity;
    int _gpuOctreeRootIndex;
    int _gpuOctreeNodeCount;
    int _gpuOctreeLeafCount;
    GpuSystemMetrics* _mappedMetricsHost;
    GpuSystemMetrics* _mappedMetricsDevice;
    std::uint64_t _metricsStepId;
    float _metricsSimTime;
    int _linearOctreeLeafCapacity;
};


#endif // BLITZAR_ENGINE_INCLUDE_PHYSICS_PARTICLESYSTEM_HPP_
