// File: engine/include/physics/Vector.inl
// Purpose: Engine implementation for the BLITZAR simulation core.

GRAVITY_HD_HOST GRAVITY_HD_DEVICE inline Vector3::Vector3() : x(0.0f), y(0.0f), z(0.0f)
{
}

/// Description: Executes the Vector3 operation.
GRAVITY_HD_HOST GRAVITY_HD_DEVICE inline Vector3::Vector3(float xValue, float yValue, float zValue)
    : x(xValue), y(yValue), z(zValue)
{
}
