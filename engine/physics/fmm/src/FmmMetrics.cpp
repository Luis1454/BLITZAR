/*
 * @file engine/physics/fmm/src/FmmMetrics.cpp
 * @brief Deterministic force-error metrics for FMM qualification.
 */

#include "FmmCpu.hpp"
#include <algorithm>
#include <cmath>

namespace bltzr_fmm {
ForceErrorMetrics measureForceError(const std::vector<Vector3>& approximate,
                                    const std::vector<Vector3>& reference)
{
    ForceErrorMetrics metrics;
    if (approximate.size() != reference.size() || approximate.empty())
        return metrics;
    std::vector<double> relativeErrors;
    relativeErrors.reserve(approximate.size());
    double referenceSquared = 0.0;
    double errorSquared = 0.0;
    double referenceMaximum = 0.0;
    double errorMaximum = 0.0;
    metrics.finite = true;
    for (std::size_t index = 0; index < approximate.size(); ++index) {
        const Vector3 delta = approximate[index] - reference[index];
        const double currentReference =
            std::sqrt(static_cast<double>(dot(reference[index], reference[index])));
        const double currentError = std::sqrt(static_cast<double>(dot(delta, delta)));
        metrics.finite =
            metrics.finite && std::isfinite(currentReference) && std::isfinite(currentError);
        referenceSquared += currentReference * currentReference;
        errorSquared += currentError * currentError;
        referenceMaximum = std::max(referenceMaximum, currentReference);
        errorMaximum = std::max(errorMaximum, currentError);
        relativeErrors.push_back(currentError / std::max(currentReference, 1.0e-12));
    }
    std::sort(relativeErrors.begin(), relativeErrors.end());
    const std::size_t p99 =
        std::min(relativeErrors.size() - 1u,
                 static_cast<std::size_t>(std::ceil(relativeErrors.size() * 0.99)) - 1u);
    metrics.relativeL2 = std::sqrt(errorSquared / std::max(referenceSquared, 1.0e-24));
    metrics.relativeLinf = errorMaximum / std::max(referenceMaximum, 1.0e-12);
    metrics.relativeP99 = relativeErrors[p99];
    return metrics;
}

StateInvariantMetrics measureStateInvariants(const std::vector<Particle>& particles,
                                             const ForceLawPolicy& policy)
{
    StateInvariantMetrics metrics;
    double potential = 0.0;
    metrics.finite = true;
    for (std::size_t left = 0; left < particles.size(); ++left) {
        const Particle& particle = particles[left];
        const Vector3 position = particle.getPosition();
        const Vector3 velocity = particle.getVelocity();
        const double mass = particle.getMass();
        metrics.totalEnergy += 0.5 * mass * static_cast<double>(dot(velocity, velocity));
        metrics.linearMomentum += velocity * static_cast<float>(mass);
        metrics.angularMomentum += Vector3(position.y * velocity.z - position.z * velocity.y,
                                           position.z * velocity.x - position.x * velocity.z,
                                           position.x * velocity.y - position.y * velocity.x) *
                                   static_cast<float>(mass);
        metrics.finite = metrics.finite && std::isfinite(mass) && std::isfinite(position.x) &&
                         std::isfinite(position.y) && std::isfinite(position.z) &&
                         std::isfinite(velocity.x) && std::isfinite(velocity.y) &&
                         std::isfinite(velocity.z);
        for (std::size_t right = left + 1u; right < particles.size(); ++right) {
            const Vector3 delta = particles[right].getPosition() - position;
            const double distance2 = static_cast<double>(dot(delta, delta)) +
                                     static_cast<double>(policy.softening) * policy.softening;
            if (distance2 > policy.minDistance2)
                potential -= mass * particles[right].getMass() / std::sqrt(distance2);
        }
    }
    metrics.totalEnergy += potential;
    metrics.finite =
        metrics.finite && std::isfinite(metrics.totalEnergy) &&
        std::isfinite(metrics.linearMomentum.x) && std::isfinite(metrics.linearMomentum.y) &&
        std::isfinite(metrics.linearMomentum.z) && std::isfinite(metrics.angularMomentum.x) &&
        std::isfinite(metrics.angularMomentum.y) && std::isfinite(metrics.angularMomentum.z);
    return metrics;
}
} // namespace bltzr_fmm
