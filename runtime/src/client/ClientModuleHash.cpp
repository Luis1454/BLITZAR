// File: runtime/src/client/ClientModuleHash.cpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#include "client/ClientModuleHash.hpp"
#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
namespace grav_module {
/// Description: Defines the ClientModuleHashLocal data or behavior contract.
class ClientModuleHashLocal final {
public:
    static bool computeFileSha256Hex(std::string_view filePath, std::string& outHexDigest,
                                     std::string& outError)
    {
        /// Description: Executes the input operation.
        std::ifstream input(std::string(filePath), std::ios::binary);
        if (!input.is_open()) {
            outError = "failed to open module for hashing: " + std::string(filePath);
            return false;
        }
        std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)),
                                        std::istreambuf_iterator<char>());
        outHexDigest = digest(bytes);
        outError.clear();
        return true;
    }

private:
    /// Description: Executes the digest operation.
    static std::string digest(const std::vector<std::uint8_t>& input)
    {
        std::array<std::uint32_t, 8u> hash{0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
                                           0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u};
        std::vector<std::uint8_t> padded = pad(input);
        for (std::size_t offset = 0u; offset < padded.size(); offset += 64u) {
            /// Description: Executes the transformBlock operation.
            transformBlock(hash, padded.data() + offset);
        }
        std::ostringstream output;
        /// Description: Executes the setfill operation.
        output << std::hex << std::setfill('0');
        for (const std::uint32_t value : hash)
            output << std::setw(8) << value;
        return output.str();
    }
    /// Description: Executes the pad operation.
    static std::vector<std::uint8_t> pad(const std::vector<std::uint8_t>& input)
    {
        std::vector<std::uint8_t> padded = input;
        padded.push_back(0x80u);
        while ((padded.size() % 64u) != 56u) {
            padded.push_back(0u);
        }
        const std::uint64_t bitLength = static_cast<std::uint64_t>(input.size()) * 8u;
        for (int shift = 56; shift >= 0; shift -= 8)
            padded.push_back(static_cast<std::uint8_t>((bitLength >> shift) & 0xffu));
        return padded;
    }
    /// Description: Executes the transformBlock operation.
    static void transformBlock(std::array<std::uint32_t, 8u>& hash, const std::uint8_t* block)
    {
        std::array<std::uint32_t, 64u> words{};
        for (std::size_t i = 0u; i < 16u; ++i) {
            const std::size_t offset = i * 4u;
            words[i] = (static_cast<std::uint32_t>(block[offset]) << 24u) |
                       (static_cast<std::uint32_t>(block[offset + 1u]) << 16u) |
                       (static_cast<std::uint32_t>(block[offset + 2u]) << 8u) |
                       static_cast<std::uint32_t>(block[offset + 3u]);
        }
        for (std::size_t i = 16u; i < words.size(); ++i) {
            words[i] = smallSigma1(words[i - 2u]) + words[i - 7u] + smallSigma0(words[i - 15u]) +
                       words[i - 16u];
        }
        std::uint32_t a = hash[0];
        std::uint32_t b = hash[1];
        std::uint32_t c = hash[2];
        std::uint32_t d = hash[3];
        std::uint32_t e = hash[4];
        std::uint32_t f = hash[5];
        std::uint32_t g = hash[6];
        std::uint32_t h = hash[7];
        for (std::size_t i = 0u; i < 64u; ++i) {
            const std::uint32_t t1 =
                h + bigSigma1(e) + choose(e, f, g) + kRoundConstants[i] + words[i];
            const std::uint32_t t2 = bigSigma0(a) + majority(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }
        hash[0] += a;
        hash[1] += b;
        hash[2] += c;
        hash[3] += d;
        hash[4] += e;
        hash[5] += f;
        hash[6] += g;
        hash[7] += h;
    }
    static std::uint32_t rotateRight(std::uint32_t value, std::uint32_t shift) noexcept
    {
        return (value >> shift) | (value << (32u - shift));
    }
    static std::uint32_t choose(std::uint32_t x, std::uint32_t y, std::uint32_t z) noexcept
    {
        return (x & y) ^ (~x & z);
    }
    static std::uint32_t majority(std::uint32_t x, std::uint32_t y, std::uint32_t z) noexcept
    {
        return (x & y) ^ (x & z) ^ (y & z);
    }
    static std::uint32_t bigSigma0(std::uint32_t value) noexcept
    {
        return rotateRight(value, 2u) ^ rotateRight(value, 13u) ^ rotateRight(value, 22u);
    }
    static std::uint32_t bigSigma1(std::uint32_t value) noexcept
    {
        return rotateRight(value, 6u) ^ rotateRight(value, 11u) ^ rotateRight(value, 25u);
    }
    static std::uint32_t smallSigma0(std::uint32_t value) noexcept
    {
        return rotateRight(value, 7u) ^ rotateRight(value, 18u) ^ (value >> 3u);
    }
    static std::uint32_t smallSigma1(std::uint32_t value) noexcept
    {
        return rotateRight(value, 17u) ^ rotateRight(value, 19u) ^ (value >> 10u);
    }
    static const std::array<std::uint32_t, 64u> kRoundConstants;
};
const std::array<std::uint32_t, 64u> ClientModuleHashLocal::kRoundConstants{
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u,
    0xab1c5ed5u, 0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu,
    0x9bdc06a7u, 0xc19bf174u, 0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu,
    0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau, 0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
    0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u, 0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu,
    0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u, 0xa2bfe8a1u, 0xa81a664bu,
    0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u, 0x19a4c116u,
    0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u,
    0xc67178f2u};
bool ClientModuleHash::computeFileSha256Hex(std::string_view filePath, std::string& outHexDigest,
                                            std::string& outError)
{
    return ClientModuleHashLocal::computeFileSha256Hex(filePath, outHexDigest, outError);
}
} // namespace grav_module
