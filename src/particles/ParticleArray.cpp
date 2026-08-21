#include "particles/ParticleArray.hpp"

#include <algorithm>
#include <limits>
#include <new>
#include <stdexcept>

namespace blitzar_particles {

ParticleArray::ParticleArray(std::size_t count) : count_(count), data_(nullptr)
{
    if (count > std::numeric_limits<std::size_t>::max() /
                     sizeof(blitzar_core::Scalar)) {
        throw std::length_error("particle array is too large");
    }
    if (count == 0) {
        return;
    }

    auto* storage = static_cast<blitzar_core::Scalar*>(::operator new[](
        count * sizeof(blitzar_core::Scalar), std::align_val_t(64)));
    data_.reset(storage);
    Fill(0.0);
}

std::size_t ParticleArray::Size() const noexcept
{
    return count_;
}

blitzar_core::Scalar* ParticleArray::Data() noexcept
{
    return data_.get();
}

const blitzar_core::Scalar* ParticleArray::Data() const noexcept
{
    return data_.get();
}

void ParticleArray::Fill(blitzar_core::Scalar value) noexcept
{
    if (count_ == 0) {
        return;
    }
    std::fill_n(data_.get(), count_, value);
}

void ParticleArray::Deleter::operator()(
    blitzar_core::Scalar* data) const noexcept
{
    ::operator delete[](data, std::align_val_t(64));
}

}  // namespace blitzar_particles
