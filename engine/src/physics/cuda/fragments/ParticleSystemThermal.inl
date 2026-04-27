/*
 * @file engine/src/physics/cuda/fragments/ParticleSystemThermal.inl
 * @author Luis1454
 * @project BLITZAR
 * @brief Physics and CUDA implementation for the deterministic simulation core.
 */

/*
 * Module: physics/cuda
 * Responsibility: Apply thermal-model configuration and energy integration helpers.
 */

/*
 * @brief Documents the set thermal parameters operation contract.
 * @param ambientTemperature Input value used by this contract.
 * @param specificHeat Input value used by this contract.
 * @param heatingCoeff Input value used by this contract.
 * @param radiationCoeff Input value used by this contract.
 * @return void ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void ParticleSystem::setThermalParameters(float ambientTemperature, float specificHeat, float heatingCoeff, float radiationCoeff)
{
    _thermalAmbientTemperature = std::max(0.0f, ambientTemperature);
    _thermalSpecificHeat = std::max(1e-6f, specificHeat);
    _thermalHeatingCoeff = std::max(0.0f, heatingCoeff);
    _thermalRadiationCoeff = std::max(0.0f, radiationCoeff);
}

/*
 * @brief Documents the apply thermal model operation contract.
 * @param deltaTime Input value used by this contract.
 * @return float ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
float ParticleSystem::applyThermalModel(float deltaTime)
{
    if (deltaTime <= 0.0f) return 0.0f;
    float radiativeExchangeStep = 0.0f;
    constexpr float kStefanBoltzmann = 5.670374419e-8f;
    constexpr float kParticleDensity = 1.0f;
    const float ambient4 = _thermalAmbientTemperature * _thermalAmbientTemperature * _thermalAmbientTemperature * _thermalAmbientTemperature;

    for (Particle &p : _particles) {
        const float mass = std::max(1e-6f, p.getMass());
        const float heatCapacity = _thermalSpecificHeat * mass;
        float temperature = std::min(std::max(0.0f, p.getTemperature()), 1e6f);
        Vector3 velocity = p.getVelocity();
        const float speed2 = dot(velocity, velocity);
        float kineticEnergy = 0.5f * mass * speed2;

        if (_thermalHeatingCoeff > 0.0f && heatCapacity > 1e-9f && kineticEnergy > 1e-12f) {
            const float requestedHeating = _thermalHeatingCoeff * p.getPressure().norm() * mass * deltaTime;
            const float convertedEnergy = std::min(std::max(0.0f, requestedHeating), kineticEnergy);
            if (convertedEnergy > 0.0f) {
                const float scale = std::sqrt(std::max(0.0f, (kineticEnergy - convertedEnergy) / kineticEnergy));
                p.setVelocity(velocity * scale);
                temperature += convertedEnergy / heatCapacity;
            }
        }

        if (_thermalRadiationCoeff > 0.0f && heatCapacity > 1e-9f) {
            const float radius = cbrtf((3.0f * mass) / (4.0f * kPi * kParticleDensity));
            const float area = 4.0f * kPi * radius * radius;
            const float t4 = powf(temperature, 4.0f);
            const float netRadiativePower = _thermalRadiationCoeff * kStefanBoltzmann * area * (t4 - ambient4);
            const float radiativeEnergy = netRadiativePower * deltaTime;
            const float nextTemperature = std::max(0.0f, temperature - (radiativeEnergy / heatCapacity));
            radiativeExchangeStep += (temperature - nextTemperature) * heatCapacity;
            temperature = nextTemperature;
        }
        p.setTemperature(temperature);
    }
    _cumulativeRadiatedEnergy += radiativeExchangeStep;
    return radiativeExchangeStep;
}
