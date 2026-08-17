/*
 * @file engine/physics/treepm/fragments/TpmShortRange.inl
 * @brief Short-range TreePM source interaction.
 */

template <typename Scalar>
Vector3 sourceAcceleration(Vector3 self, const Particle& source, const ForceLawPolicy& policy)
{
    const Vector3 delta = source.getPosition() - self;
    const Scalar distance2 = static_cast<Scalar>(delta.x) * delta.x +
                             static_cast<Scalar>(delta.y) * delta.y +
                             static_cast<Scalar>(delta.z) * delta.z +
                             static_cast<Scalar>(policy.softening) * policy.softening;
    if (distance2 <= static_cast<Scalar>(policy.minDistance2)) {
        return Vector3();
    }
    const Scalar inverseDistance = static_cast<Scalar>(1.0) / std::sqrt(distance2);
    Scalar shortRangeWeight = static_cast<Scalar>(1.0);
    if (policy.treePmShortRangeScale > 0.0f) {
        const Scalar distance = static_cast<Scalar>(1.0) / inverseDistance;
        const Scalar splitScale = static_cast<Scalar>(policy.treePmShortRangeScale);
        const Scalar argument = static_cast<Scalar>(0.5) * distance / splitScale;
        shortRangeWeight =
            std::erfc(argument) +
            distance / (splitScale * std::sqrt(static_cast<Scalar>(3.14159265358979323846))) *
                std::exp(-argument * argument);
    }
    const Scalar scale = static_cast<Scalar>(source.getMass()) * inverseDistance * inverseDistance *
                         inverseDistance * shortRangeWeight;
    return Vector3(static_cast<float>(static_cast<Scalar>(delta.x) * scale),
                   static_cast<float>(static_cast<Scalar>(delta.y) * scale),
                   static_cast<float>(static_cast<Scalar>(delta.z) * scale));
}
