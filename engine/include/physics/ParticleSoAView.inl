// File: engine/include/physics/ParticleSoAView.inl
// Purpose: Engine implementation for the BLITZAR simulation core.

GRAVITY_HD_HOST GRAVITY_HD_DEVICE inline Vector3 getSoAPosition(ParticleSoAView view, int i)
{
    return Vector3{view.posX[i], view.posY[i], view.posZ[i]};
}

/// Description: Executes the setSoAPosition operation.
GRAVITY_HD_HOST GRAVITY_HD_DEVICE inline void setSoAPosition(ParticleSoAView view, int i, Vector3 p)
{
    view.posX[i] = p.x;
    view.posY[i] = p.y;
    view.posZ[i] = p.z;
}

/// Description: Executes the getSoAVelocity operation.
GRAVITY_HD_HOST GRAVITY_HD_DEVICE inline Vector3 getSoAVelocity(ParticleSoAView view, int i)
{
    return Vector3{view.velX[i], view.velY[i], view.velZ[i]};
}

/// Description: Executes the setSoAVelocity operation.
GRAVITY_HD_HOST GRAVITY_HD_DEVICE inline void setSoAVelocity(ParticleSoAView view, int i, Vector3 v)
{
    view.velX[i] = v.x;
    view.velY[i] = v.y;
    view.velZ[i] = v.z;
}

/// Description: Executes the setSoAPressure operation.
GRAVITY_HD_HOST GRAVITY_HD_DEVICE inline void setSoAPressure(ParticleSoAView view, int i, Vector3 p)
{
    view.pressX[i] = p.x;
    view.pressY[i] = p.y;
    view.pressZ[i] = p.z;
}

/// Description: Executes the getSoAPressure operation.
GRAVITY_HD_HOST GRAVITY_HD_DEVICE inline Vector3 getSoAPressure(ParticleSoAView view, int i)
{
    return Vector3{view.pressX[i], view.pressY[i], view.pressZ[i]};
}
