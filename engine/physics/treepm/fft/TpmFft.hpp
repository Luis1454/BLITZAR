/*
 * @file engine/physics/treepm/fft/TpmFft.hpp
 * @brief Host-side FFT stage contract for TreePM.
 */

#ifndef BLITZAR_ENGINE_PHYSICS_TREEPM_FFT_TPMFFT_HPP_
#define BLITZAR_ENGINE_PHYSICS_TREEPM_FFT_TPMFFT_HPP_

#include <cstdint>

struct TpmFftGridShape final {
    std::uint32_t x = 0u;
    std::uint32_t y = 0u;
    std::uint32_t z = 0u;
};

#endif // BLITZAR_ENGINE_PHYSICS_TREEPM_FFT_TPMFFT_HPP_
