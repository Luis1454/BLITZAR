/*
 * @file engine/physics/treepm/src/fragments/TpmMath.inl
 * @brief Scalar math and radix-2 FFT helpers for the CPU TreePM path.
 */

template <typename Scalar> constexpr Scalar kTwoPi = static_cast<Scalar>(6.2831853071795864769);

template <typename Scalar> constexpr Scalar kFourPi = static_cast<Scalar>(12.566370614359172);

int gridIndex(int x, int y, int z, int gridSize)
{
    return (z * gridSize + y) * gridSize + x;
}

template <typename Scalar> Scalar modifiedBesselK1(Scalar value)
{
    const Scalar x = std::max(value, static_cast<Scalar>(1.0e-4));
    if (x <= static_cast<Scalar>(2.0)) {
        const Scalar y = x * x * static_cast<Scalar>(0.25);
        Scalar i1Poly = static_cast<Scalar>(0.00032411);
        i1Poly = static_cast<Scalar>(0.00301532) + y * i1Poly;
        i1Poly = static_cast<Scalar>(0.02658733) + y * i1Poly;
        i1Poly = static_cast<Scalar>(0.15084934) + y * i1Poly;
        i1Poly = static_cast<Scalar>(0.51498869) + y * i1Poly;
        i1Poly = static_cast<Scalar>(0.87890594) + y * i1Poly;
        const Scalar i1 = x * static_cast<Scalar>(0.5) * (static_cast<Scalar>(1.0) + y * i1Poly);
        Scalar k1Poly = static_cast<Scalar>(-0.00004686);
        k1Poly = static_cast<Scalar>(-0.00110404) + y * k1Poly;
        k1Poly = static_cast<Scalar>(-0.01919402) + y * k1Poly;
        k1Poly = static_cast<Scalar>(-0.18156897) + y * k1Poly;
        k1Poly = static_cast<Scalar>(-0.67278579) + y * k1Poly;
        k1Poly = static_cast<Scalar>(0.15443144) + y * k1Poly;
        return std::log(x * static_cast<Scalar>(0.5)) * i1 +
               (static_cast<Scalar>(1.0) / x) * (static_cast<Scalar>(1.0) + y * k1Poly);
    }
    const Scalar y = static_cast<Scalar>(2.0) / x;
    Scalar asymptotic = static_cast<Scalar>(-0.00068245);
    asymptotic = static_cast<Scalar>(0.00325614) + y * asymptotic;
    asymptotic = static_cast<Scalar>(-0.00780353) + y * asymptotic;
    asymptotic = static_cast<Scalar>(0.01504268) + y * asymptotic;
    asymptotic = static_cast<Scalar>(-0.03655620) + y * asymptotic;
    asymptotic = static_cast<Scalar>(0.23498619) + y * asymptotic;
    return std::exp(-x) * (static_cast<Scalar>(1.0) / std::sqrt(x)) *
           static_cast<Scalar>(1.25331414) * (static_cast<Scalar>(1.0) + y * asymptotic);
}

template <typename Scalar> Scalar sinc(Scalar value)
{
    return std::fabs(value) < static_cast<Scalar>(1.0e-5) ? static_cast<Scalar>(1.0)
                                                          : std::sin(value) / value;
}

template <typename Scalar>
void fft1d(std::vector<std::complex<Scalar>>& values, int start, int stride, int size, bool inverse)
{
    for (int i = 1, j = 0; i < size; ++i) {
        int bit = size >> 1;
        for (; (j & bit) != 0; bit >>= 1) {
            j ^= bit;
        }
        j ^= bit;
        if (i < j) {
            std::swap(values[static_cast<std::size_t>(start + i * stride)],
                      values[static_cast<std::size_t>(start + j * stride)]);
        }
    }

    for (int length = 2; length <= size; length <<= 1) {
        const Scalar angle = (inverse ? static_cast<Scalar>(1.0) : static_cast<Scalar>(-1.0)) *
                             kTwoPi<Scalar> / static_cast<Scalar>(length);
        const std::complex<Scalar> step(std::cos(angle), std::sin(angle));
        for (int offset = 0; offset < size; offset += length) {
            std::complex<Scalar> factor(static_cast<Scalar>(1.0), static_cast<Scalar>(0.0));
            const int halfLength = length >> 1;
            for (int i = 0; i < halfLength; ++i) {
                const std::size_t evenIndex =
                    static_cast<std::size_t>(start + (offset + i) * stride);
                const std::size_t oddIndex =
                    static_cast<std::size_t>(start + (offset + i + halfLength) * stride);
                const std::complex<Scalar> even = values[evenIndex];
                const std::complex<Scalar> odd = factor * values[oddIndex];
                values[evenIndex] = even + odd;
                values[oddIndex] = even - odd;
                factor *= step;
            }
        }
    }
}

template <typename Scalar>
void fft3d(std::vector<std::complex<Scalar>>& values, int size, bool inverse)
{
#pragma omp parallel for schedule(static)
    for (int z = 0; z < size; ++z) {
        for (int y = 0; y < size; ++y) {
            fft1d<Scalar>(values, gridIndex(0, y, z, size), 1, size, inverse);
        }
    }
#pragma omp parallel for schedule(static)
    for (int z = 0; z < size; ++z) {
        for (int x = 0; x < size; ++x) {
            fft1d<Scalar>(values, gridIndex(x, 0, z, size), size, size, inverse);
        }
    }
#pragma omp parallel for schedule(static)
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            fft1d<Scalar>(values, gridIndex(x, y, 0, size), size * size, size, inverse);
        }
    }
    if (inverse) {
        const Scalar inverseCells =
            static_cast<Scalar>(1.0) / static_cast<Scalar>(size * size * size);
        for (std::complex<Scalar>& value : values) {
            value *= inverseCells;
        }
    }
}
