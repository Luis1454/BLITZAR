/*
 * @file engine/include/physics/Particle.inl
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Inline host/device particle operations shared by CPU and CUDA builds.
 */

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline Particle::Particle()
    : _position(Vector3{0.0f, 0.0f, 0.0f}),
      _velocity(Vector3{0.0f, 0.0f, 0.0f}),
      _pressure(Vector3{0.0f, 0.0f, 0.0f}),
      _force(Vector3{0.0f, 0.0f, 0.0f}),
      _density(0.0f),
      _mass(Particle::kDefaultMass),
      _temperature(0.0f)
{
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline Particle::~Particle()
{
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline void Particle::update(float deltaTime)
{
    _velocity += _force * deltaTime;
    _position += _velocity * deltaTime;
    _force = Vector3{0.0f, 0.0f, 0.0f};
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline void Particle::setMass(float mass)
{
    _mass = mass;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline void Particle::move(Vector3 force)
{
    _position += force;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline void Particle::bounce(Vector3 normal, float dt)
{
    _position -= _velocity * dt;
    _velocity -= normal * 2.0f * dot(_velocity, normal);
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline void Particle::applyForce(Vector3 force)
{
    _force += force;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline void Particle::setDensity(float density)
{
    _density = density;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline void Particle::setPosition(Vector3 position)
{
    _position = position;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline void Particle::setVelocity(Vector3 velocity)
{
    _velocity = velocity;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline void Particle::setPressure(Vector3 pressure)
{
    _pressure = pressure;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline void Particle::setTemperature(float temperature)
{
    _temperature = temperature;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline Vector3 Particle::getPressure() const
{
    return _pressure;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline Vector3 Particle::getPosition() const
{
    return _position;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline Vector3 Particle::getVelocity() const
{
    return _velocity;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline float Particle::getDensity() const
{
    return _density;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline float Particle::getMass() const
{
    return _mass;
}

BLITZAR_HD_HOST BLITZAR_HD_DEVICE inline float Particle::getTemperature() const
{
    return _temperature;
}
