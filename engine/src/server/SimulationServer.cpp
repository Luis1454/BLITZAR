#include "server/SimulationServer.hpp"
#include "server/SimulationInitConfig.hpp"
#include "config/EnvUtils.hpp"
#include "config/SimulationModes.hpp"
#include "platform/PlatformPaths.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <cuda_runtime.h>
static std::string toLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

static std::string trim(std::string value)
{
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) {
        return std::isspace(c) != 0;
    }).base();
    if (begin >= end) {
        return {};
    }
    return std::string(begin, end);
}

std::string normalizeSnapshotFormat(std::string format)
{
    format = toLower(std::move(format));
    if (format == "binary") {
        return "bin";
    }
    if (format == "vtkb" || format == "vtk-bin") {
        return "vtk_binary";
    }
    if (format == "vtk_ascii") {
        return "vtk";
    }
    return format;
}

ParticleSystem::SolverMode solverModeFromCanonicalName(std::string_view name)
{
    if (name == grav_modes::kSolverOctreeCpu) {
        return ParticleSystem::SolverMode::OctreeCpu;
    }
    if (name == grav_modes::kSolverOctreeGpu) {
        return ParticleSystem::SolverMode::OctreeGpu;
    }
    return ParticleSystem::SolverMode::PairwiseCuda;
}

ParticleSystem::IntegratorMode integratorModeFromCanonicalName(std::string_view name)
{
    if (name == grav_modes::kIntegratorRk4) {
        return ParticleSystem::IntegratorMode::Rk4;
    }
    return ParticleSystem::IntegratorMode::Euler;
}

std::string_view solverLabel(ParticleSystem::SolverMode mode)
{
    switch (mode) {
        case ParticleSystem::SolverMode::OctreeCpu:
            return "octree_cpu";
        case ParticleSystem::SolverMode::OctreeGpu:
            return "octree_gpu";
        case ParticleSystem::SolverMode::PairwiseCuda:
        default:
            return "pairwise_cuda";
    }
}

constexpr std::size_t kMaxImportedParticles = 2'000'000;
constexpr std::uint32_t kPairwiseRealtimeParticleLimit = 20'000u;

bool readRawBytes(std::istream &in, std::byte *data, std::size_t size)
{
    return static_cast<bool>(in.read(reinterpret_cast<char *>(data), static_cast<std::streamsize>(size)));
}

bool writeRawBytes(std::ostream &out, const std::byte *data, std::size_t size)
{
    out.write(reinterpret_cast<const char *>(data), static_cast<std::streamsize>(size));
    return static_cast<bool>(out);
}

std::string readEnvironment(std::string_view key)
{
    const std::optional<std::string> value = grav_env::get(key);
    return value.value_or(std::string{});
}

bool isValidImportedParticleCount(std::size_t count)
{
    return count >= 2 && count <= kMaxImportedParticles;
}

bool isAutoSolverFallbackEnabled()
{
    const std::string raw = readEnvironment("GRAVITY_AUTO_SOLVER_FALLBACK");
    if (raw.empty()) {
        return false;
    }
    const std::string v = toLower(trim(raw));
    return v == "1" || v == "true" || v == "on" || v == "yes";
}

bool shouldForceCudaFailureOnceForTesting(std::string_view solver)
{
    if (solver != grav_modes::kSolverPairwiseCuda && solver != grav_modes::kSolverOctreeGpu) {
        return false;
    }
    const std::string raw = readEnvironment("GRAVITY_TEST_FORCE_CUDA_FAIL_ONCE");
    if (raw.empty()) {
        return false;
    }
    const std::string v = toLower(trim(raw));
    if (!(v == "1" || v == "true" || v == "on" || v == "yes")) {
        return false;
    }
    static std::atomic<bool> injected{false};
    bool expected = false;
    return injected.compare_exchange_strong(expected, true, std::memory_order_relaxed);
}

bool coerceConfigSolverIntegratorCompatibility(std::string &solver, std::string &integrator, std::string_view source)
{
    if (!grav_modes::isSupportedSolverIntegratorPair(solver, integrator)) {
        std::cerr << "[server] " << source
                  << ": solver octree_gpu requires integrator euler, using euler\n";
        integrator.assign(grav_modes::kIntegratorEuler);
        return true;
    }
    return false;
}

void logEffectiveExecutionModes(
    std::string_view solver,
    std::string_view integrator,
    float theta,
    float softening,
    bool sphEnabled)
{
    std::cout << "[server] active solver=" << solver
              << " integrator=" << integrator;
    if (solver == grav_modes::kSolverOctreeCpu || solver == grav_modes::kSolverOctreeGpu) {
        std::cout << " theta=" << theta
                  << " softening=" << softening;
    }
    std::cout << " sph=" << (sphEnabled ? "on" : "off") << "\n";
}

std::string defaultExportPath(const std::string &directory, const std::string &format, std::uint64_t step)
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    const std::tm tm = grav_platform::localTime(nowTime);
    std::string extension = normalizeSnapshotFormat(format);
    if (extension == "vtk_binary") {
        extension = "vtk";
    }
    std::ostringstream fileName;
    fileName << "sim_" << std::put_time(&tm, "%Y%m%d_%H%M%S") << "_s" << step << "." << extension;
    std::filesystem::path outPath = std::filesystem::path(directory) / fileName.str();
    return outPath.string();
}

std::string guessFormatFromPath(const std::string &path)
{
    const std::filesystem::path p(path);
    std::string ext = toLower(p.extension().string());
    if (!ext.empty() && ext[0] == '.') {
        ext.erase(ext.begin());
    }
    if (ext == "binary" || ext == "nbin") {
        return "bin";
    }
    if (ext == "vtkb") {
        return "vtk_binary";
    }
    return ext;
}

bool readBeU32(std::istream &in, std::uint32_t &outValue)
{
    std::array<std::byte, 4> bytes{std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}};
    if (!readRawBytes(in, bytes.data(), bytes.size())) {
        return false;
    }
    outValue = (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[0])) << 24)
        | (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[1])) << 16)
        | (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[2])) << 8)
        | static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[3]));
    return true;
}

bool readBeI32(std::istream &in, std::int32_t &outValue)
{
    std::uint32_t unsignedValue = 0u;
    if (!readBeU32(in, unsignedValue)) {
        return false;
    }
    outValue = static_cast<std::int32_t>(unsignedValue);
    return true;
}

bool readBeF32(std::istream &in, float &outValue)
{
    std::uint32_t bits = 0u;
    if (!readBeU32(in, bits)) {
        return false;
    }
    std::memcpy(&outValue, &bits, sizeof(float));
    return true;
}

void writeBeU32(std::ostream &out, std::uint32_t value)
{
    const std::array<std::byte, 4> bytes{
        std::byte{static_cast<unsigned char>((value >> 24) & 0xFFu)},
        std::byte{static_cast<unsigned char>((value >> 16) & 0xFFu)},
        std::byte{static_cast<unsigned char>((value >> 8) & 0xFFu)},
        std::byte{static_cast<unsigned char>(value & 0xFFu)}
    };
    (void)writeRawBytes(out, bytes.data(), bytes.size());
}

void writeBeI32(std::ostream &out, std::int32_t value)
{
    writeBeU32(out, static_cast<std::uint32_t>(value));
}

void writeBeF32(std::ostream &out, float value)
{
    std::uint32_t bits = 0u;
    std::memcpy(&bits, &value, sizeof(float));
    writeBeU32(out, bits);
}

void consumeOptionalLineBreak(std::istream &in)
{
    const int next = in.peek();
    if (next == '\n') {
        in.get();
        return;
    }
    if (next == '\r') {
        in.get();
        if (in.peek() == '\n') {
            in.get();
        }
    }
}

struct BinarySnapshotHeader {
    char magic[8];
    std::uint32_t version;
    std::uint32_t count;
};

struct BinarySnapshotParticle {
    float px;
    float py;
    float pz;
    float vx;
    float vy;
    float vz;
    float mass;
    float temperature;
};

constexpr char kBinarySnapshotMagic[8] = {'N', 'B', 'S', 'I', 'M', 'B', 'I', 'N'};
constexpr std::uint32_t kBinarySnapshotVersion = 1u;

bool parseBinarySnapshot(const std::string &inputPath, std::vector<Particle> &outParticles)
{
    std::ifstream in(inputPath, std::ios::binary);
    if (!in.is_open()) {
        return false;
    }

    BinarySnapshotHeader header{};
    std::array<std::byte, sizeof(BinarySnapshotHeader)> headerBytes{};
    if (!readRawBytes(in, headerBytes.data(), headerBytes.size())) {
        return false;
    }
    std::memcpy(&header, headerBytes.data(), sizeof(header));
    if (std::memcmp(header.magic, kBinarySnapshotMagic, sizeof(kBinarySnapshotMagic)) != 0) {
        return false;
    }
    if (header.version != kBinarySnapshotVersion || !isValidImportedParticleCount(header.count)) {
        return false;
    }

    in.seekg(0, std::ios::end);
    const std::streamoff fileSize = in.tellg();
    if (fileSize < 0) {
        return false;
    }
    const std::size_t expectedSize =
        sizeof(BinarySnapshotHeader) + static_cast<std::size_t>(header.count) * sizeof(BinarySnapshotParticle);
    if (static_cast<std::size_t>(fileSize) < expectedSize) {
        return false;
    }
    in.seekg(static_cast<std::streamoff>(sizeof(BinarySnapshotHeader)), std::ios::beg);
    if (!in) {
        return false;
    }

    outParticles.clear();
    outParticles.reserve(header.count);
    for (std::uint32_t i = 0; i < header.count; ++i) {
        BinarySnapshotParticle rec{};
        std::array<std::byte, sizeof(BinarySnapshotParticle)> recBytes{};
        if (!readRawBytes(in, recBytes.data(), recBytes.size())) {
            outParticles.clear();
            return false;
        }
        std::memcpy(&rec, recBytes.data(), sizeof(rec));
        Particle p;
        p.setPosition(Vector3(rec.px, rec.py, rec.pz));
        p.setVelocity(Vector3(rec.vx, rec.vy, rec.vz));
        p.setPressure(Vector3(0.0f, 0.0f, 0.0f));
        p.setDensity(0.0f);
        if (rec.mass > 0.0f) {
            p.setMass(rec.mass);
        }
        p.setTemperature(std::max(0.0f, rec.temperature));
        outParticles.push_back(p);
    }
    return outParticles.size() >= 2;
}

bool parseXyzSnapshot(const std::string &inputPath, std::vector<Particle> &outParticles)
{
    std::ifstream in(inputPath);
    if (!in.is_open()) {
        return false;
    }

    std::size_t expectedCount = 0;
    {
        std::string firstLine;
        if (!std::getline(in, firstLine)) {
            return false;
        }
        std::istringstream iss(firstLine);
        iss >> expectedCount;
    }
    if (expectedCount > kMaxImportedParticles) {
        return false;
    }

    std::string commentLine;
    std::getline(in, commentLine);

    outParticles.clear();
    outParticles.reserve(expectedCount > 0 ? expectedCount : 128);

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        std::istringstream iss(line);
        std::vector<std::string> tokens;
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }
        if (tokens.empty()) {
            continue;
        }

        std::size_t offset = 0;
        auto tryParseFloat = [](const std::string &value, float &out) {
            std::istringstream conv(value);
            conv >> out;
            return static_cast<bool>(conv);
        };

        float x = 0.0f;
        if (!tryParseFloat(tokens[0], x)) {
            offset = 1;
        }
        if (tokens.size() < offset + 3) {
            continue;
        }

        float y = 0.0f;
        float z = 0.0f;
        if (!tryParseFloat(tokens[offset + 0], x)
            || !tryParseFloat(tokens[offset + 1], y)
            || !tryParseFloat(tokens[offset + 2], z)) {
            continue;
        }

        Particle p;
        p.setPosition(Vector3(x, y, z));
        p.setVelocity(Vector3(0.0f, 0.0f, 0.0f));
        p.setTemperature(0.0f);
        if (tokens.size() >= offset + 4) {
            float m = 0.0f;
            if (tryParseFloat(tokens[offset + 3], m) && m > 0.0f) {
                p.setMass(m);
            }
        }
        if (tokens.size() >= offset + 5) {
            float t = 0.0f;
            if (tryParseFloat(tokens[offset + 4], t)) {
                p.setTemperature(std::max(0.0f, t));
            }
        }
        outParticles.push_back(p);
        if (outParticles.size() > kMaxImportedParticles) {
            outParticles.clear();
            return false;
        }
    }

    return outParticles.size() >= 2;
}

bool parseVtkSnapshot(const std::string &inputPath, std::vector<Particle> &outParticles)
{
    auto validatePointCount = [](std::size_t pointCount) {
        return isValidImportedParticleCount(pointCount);
    };

    auto finalizeParticles = [&outParticles](
                                 const std::vector<Vector3> &positions,
                                 const std::vector<float> &masses,
                                 const std::vector<float> &temperatures,
                                 const std::vector<Vector3> &velocities) {
        if (!isValidImportedParticleCount(positions.size())) {
            return false;
        }
        outParticles.resize(positions.size());
        for (std::size_t i = 0; i < positions.size(); ++i) {
            Particle p;
            p.setPosition(positions[i]);
            p.setVelocity(i < velocities.size() ? velocities[i] : Vector3(0.0f, 0.0f, 0.0f));
            p.setPressure(Vector3(0.0f, 0.0f, 0.0f));
            p.setDensity(0.0f);
            p.setTemperature(0.0f);
            if (i < masses.size() && masses[i] > 0.0f) {
                p.setMass(masses[i]);
            }
            if (i < temperatures.size()) {
                p.setTemperature(std::max(0.0f, temperatures[i]));
            }
            outParticles[i] = p;
        }
        return true;
    };

    auto parseAsciiPayload = [&](std::istream &in) {
        std::size_t pointCount = 0;
        std::vector<Vector3> positions;
        std::vector<float> masses;
        std::vector<float> temperatures;
        std::vector<Vector3> velocities;
        std::string token;

        auto skipScalars = [&in](std::size_t count) {
            float discard = 0.0f;
            for (std::size_t i = 0; i < count; ++i) {
                if (!(in >> discard)) {
                    return false;
                }
            }
            return true;
        };
        auto skipVectors = [&in](std::size_t count) {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            for (std::size_t i = 0; i < count; ++i) {
                if (!(in >> x >> y >> z)) {
                    return false;
                }
            }
            return true;
        };

        while (in >> token) {
            if (token == "POINTS") {
                std::string type;
                in >> pointCount >> type;
                if (!validatePointCount(pointCount)) {
                    return false;
                }
                positions.resize(pointCount);
                for (std::size_t i = 0; i < pointCount; ++i) {
                    float x = 0.0f;
                    float y = 0.0f;
                    float z = 0.0f;
                    if (!(in >> x >> y >> z)) {
                        return false;
                    }
                    positions[i] = Vector3(x, y, z);
                }
                continue;
            }
            if (token == "SCALARS" && pointCount > 0) {
                std::string name;
                std::string type;
                int components = 1;
                in >> name >> type;
                if (!(in >> components)) {
                    in.clear();
                    components = 1;
                }
                std::string lookup;
                std::string tableName;
                in >> lookup >> tableName;
                const std::size_t scalarCount = pointCount * static_cast<std::size_t>(std::max(1, components));
                if (toLower(name) == "mass" && components == 1) {
                    masses.resize(pointCount);
                    for (std::size_t i = 0; i < pointCount; ++i) {
                        if (!(in >> masses[i])) {
                            return false;
                        }
                    }
                } else if (toLower(name) == "temperature" && components == 1) {
                    temperatures.resize(pointCount);
                    for (std::size_t i = 0; i < pointCount; ++i) {
                        if (!(in >> temperatures[i])) {
                            return false;
                        }
                    }
                } else {
                    if (!skipScalars(scalarCount)) {
                        return false;
                    }
                }
                continue;
            }
            if (token == "VECTORS" && pointCount > 0) {
                std::string name;
                std::string type;
                in >> name >> type;
                if (toLower(name) == "velocity") {
                    velocities.resize(pointCount);
                    for (std::size_t i = 0; i < pointCount; ++i) {
                        float x = 0.0f;
                        float y = 0.0f;
                        float z = 0.0f;
                        if (!(in >> x >> y >> z)) {
                            return false;
                        }
                        velocities[i] = Vector3(x, y, z);
                    }
                } else {
                    if (!skipVectors(pointCount)) {
                        return false;
                    }
                }
                continue;
            }
        }

        return finalizeParticles(positions, masses, temperatures, velocities);
    };

    auto parseBinaryPayload = [&](std::istream &in) {
        std::size_t pointCount = 0;
        std::vector<Vector3> positions;
        std::vector<float> masses;
        std::vector<float> temperatures;
        std::vector<Vector3> velocities;
        std::string line;

        while (std::getline(in, line)) {
            const std::string stripped = trim(line);
            if (stripped.empty()) {
                continue;
            }

            std::istringstream iss(stripped);
            std::string keyword;
            iss >> keyword;
            if (keyword == "POINTS") {
                std::string type;
                iss >> pointCount >> type;
                if (!validatePointCount(pointCount)) {
                    return false;
                }
                positions.resize(pointCount);
                for (std::size_t i = 0; i < pointCount; ++i) {
                    float x = 0.0f;
                    float y = 0.0f;
                    float z = 0.0f;
                    if (!readBeF32(in, x) || !readBeF32(in, y) || !readBeF32(in, z)) {
                        return false;
                    }
                    positions[i] = Vector3(x, y, z);
                }
                consumeOptionalLineBreak(in);
                continue;
            }
            if (keyword == "VERTICES") {
                std::size_t cellCount = 0;
                std::size_t totalInts = 0;
                iss >> cellCount >> totalInts;
                std::int32_t discard = 0;
                for (std::size_t i = 0; i < totalInts; ++i) {
                    if (!readBeI32(in, discard)) {
                        return false;
                    }
                }
                consumeOptionalLineBreak(in);
                continue;
            }
            if (keyword == "POINT_DATA") {
                continue;
            }
            if (keyword == "SCALARS" && pointCount > 0) {
                std::string name;
                std::string type;
                int components = 1;
                iss >> name >> type;
                if (!(iss >> components)) {
                    components = 1;
                }
                std::string lookupLine;
                if (!std::getline(in, lookupLine)) {
                    return false;
                }
                const std::size_t scalarCount = pointCount * static_cast<std::size_t>(std::max(1, components));
                if (toLower(name) == "mass" && components == 1) {
                    masses.resize(pointCount);
                    for (std::size_t i = 0; i < pointCount; ++i) {
                        if (!readBeF32(in, masses[i])) {
                            return false;
                        }
                    }
                } else if (toLower(name) == "temperature" && components == 1) {
                    temperatures.resize(pointCount);
                    for (std::size_t i = 0; i < pointCount; ++i) {
                        if (!readBeF32(in, temperatures[i])) {
                            return false;
                        }
                    }
                } else {
                    float discard = 0.0f;
                    for (std::size_t i = 0; i < scalarCount; ++i) {
                        if (!readBeF32(in, discard)) {
                            return false;
                        }
                    }
                }
                consumeOptionalLineBreak(in);
                continue;
            }
            if (keyword == "VECTORS" && pointCount > 0) {
                std::string name;
                std::string type;
                iss >> name >> type;
                if (toLower(name) == "velocity") {
                    velocities.resize(pointCount);
                    for (std::size_t i = 0; i < pointCount; ++i) {
                        float x = 0.0f;
                        float y = 0.0f;
                        float z = 0.0f;
                        if (!readBeF32(in, x) || !readBeF32(in, y) || !readBeF32(in, z)) {
                            return false;
                        }
                        velocities[i] = Vector3(x, y, z);
                    }
                } else {
                    float discard = 0.0f;
                    for (std::size_t i = 0; i < pointCount * 3; ++i) {
                        if (!readBeF32(in, discard)) {
                            return false;
                        }
                    }
                }
                consumeOptionalLineBreak(in);
                continue;
            }
        }

        return finalizeParticles(positions, masses, temperatures, velocities);
    };

    std::ifstream in(inputPath, std::ios::binary);
    if (!in.is_open()) {
        return false;
    }

    std::string line1;
    std::string line2;
    std::string line3;
    std::string line4;
    if (!std::getline(in, line1)
        || !std::getline(in, line2)
        || !std::getline(in, line3)
        || !std::getline(in, line4)) {
        return false;
    }

    const std::string encoding = toLower(trim(line3));
    if (encoding == "binary") {
        return parseBinaryPayload(in);
    }
    return parseAsciiPayload(in);
}

bool buildGeneratedState(std::vector<Particle> &outParticles, std::uint32_t particleCount, const InitialStateConfig &config)
{
    outParticles.clear();
    const std::uint32_t count = std::max<std::uint32_t>(2u, particleCount);
    const std::string mode = toLower(config.mode);

    std::mt19937 rng(config.seed);
    const float centralMass = std::max(1e-6f, config.centralMass);
    const float velocityTemperature = std::max(0.0f, config.velocityTemperature);
    const float particleTemperature = std::max(0.0f, config.particleTemperature);
    const Vector3 centralPos(config.centralX, config.centralY, config.centralZ);
    const Vector3 centralVel(config.centralVx, config.centralVy, config.centralVz);

    auto applyThermalVelocity = [&](Particle &p) {
        if (velocityTemperature <= 0.0f) {
            return;
        }
        const float mass = std::max(1e-6f, p.getMass());
        float sigma = std::sqrt(velocityTemperature / mass) * 0.005f;
        sigma = std::min(sigma, 2.5f);
        if (sigma <= 0.0f) {
            return;
        }
        std::normal_distribution<float> thermalDist(0.0f, sigma);
        const Vector3 v = p.getVelocity();
        p.setVelocity(Vector3(
            v.x + thermalDist(rng),
            v.y + thermalDist(rng),
            v.z + thermalDist(rng)
        ));
    };

    auto addCentralBody = [&]() {
        if (!config.includeCentralBody) {
            return;
        }
        Particle central;
        central.setMass(centralMass);
        central.setPosition(centralPos);
        central.setVelocity(centralVel);
        central.setPressure(Vector3(0.0f, 0.0f, 0.0f));
        central.setDensity(0.0f);
        central.setTemperature(particleTemperature);
        outParticles.push_back(central);
    };

    auto finalizeParticle = [&](Particle &particle) {
        applyThermalVelocity(particle);
        particle.setPressure(Vector3(0.0f, 0.0f, 0.0f));
        particle.setDensity(0.0f);
        particle.setTemperature(particleTemperature);
    };

    if (mode == "two_body") {
        const float separation = std::max(0.2f, config.cloudHalfExtent);
        const float mass = std::max(1e-6f, config.particleMass);
        const float radius = 0.5f * separation;
        const float orbitalSpeed = std::sqrt(mass / std::max(2.0f * separation, 1e-6f))
            * std::max(0.0f, config.velocityScale);

        Particle left;
        left.setMass(mass);
        left.setPosition(centralPos + Vector3(-radius, 0.0f, 0.0f));
        left.setVelocity(centralVel + Vector3(0.0f, -orbitalSpeed, 0.0f));
        finalizeParticle(left);
        outParticles.push_back(left);

        Particle right;
        right.setMass(mass);
        right.setPosition(centralPos + Vector3(radius, 0.0f, 0.0f));
        right.setVelocity(centralVel + Vector3(0.0f, orbitalSpeed, 0.0f));
        finalizeParticle(right);
        outParticles.push_back(right);
        return true;
    }

    if (mode == "three_body") {
        const float scale = std::max(0.1f, config.cloudHalfExtent);
        const float mass = std::max(1e-6f, config.particleMass);
        const float speedScale = std::max(0.0f, config.velocityScale) / std::sqrt(scale);
        constexpr float kX = 0.97000436f;
        constexpr float kY = 0.24308753f;
        constexpr float kVx = 0.46620368f;
        constexpr float kVy = 0.43236572f;
        const Vector3 positions[] = {
            Vector3(-kX * scale, kY * scale, 0.0f),
            Vector3(kX * scale, -kY * scale, 0.0f),
            Vector3(0.0f, 0.0f, 0.0f)
        };
        const Vector3 velocities[] = {
            Vector3(kVx * speedScale, kVy * speedScale, 0.0f),
            Vector3(kVx * speedScale, kVy * speedScale, 0.0f),
            Vector3(-2.0f * kVx * speedScale, -2.0f * kVy * speedScale, 0.0f)
        };
        for (int index = 0; index < 3; index += 1) {
            Particle particle;
            particle.setMass(mass);
            particle.setPosition(centralPos + positions[index]);
            particle.setVelocity(centralVel + velocities[index]);
            finalizeParticle(particle);
            outParticles.push_back(particle);
        }
        return true;
    }

    if (mode == "plummer_sphere") {
        const float scale = std::max(0.1f, config.cloudHalfExtent);
        const float totalMass = std::max(1e-6f, config.particleMass * static_cast<float>(count));
        const float mass = std::max(1e-6f, totalMass / static_cast<float>(count));
        const float sigma = std::sqrt(totalMass / std::max(6.0f * scale, 1e-6f)) * std::max(0.0f, config.velocityScale);
        std::uniform_real_distribution<float> unitDist(1e-4f, 0.9999f);
        std::uniform_real_distribution<float> azimuthDist(0.0f, 2.0f * 3.1415926535f);
        std::uniform_real_distribution<float> cosThetaDist(-1.0f, 1.0f);
        std::normal_distribution<float> velDist(0.0f, sigma);
        Vector3 meanPosition(0.0f, 0.0f, 0.0f);
        Vector3 meanVelocity(0.0f, 0.0f, 0.0f);

        while (outParticles.size() < count) {
            const float u = unitDist(rng);
            const float radius = scale / std::sqrt(std::pow(u, -2.0f / 3.0f) - 1.0f);
            const float azimuth = azimuthDist(rng);
            const float cosTheta = cosThetaDist(rng);
            const float sinTheta = std::sqrt(std::max(0.0f, 1.0f - cosTheta * cosTheta));
            const Vector3 offset(
                radius * sinTheta * std::cos(azimuth),
                radius * sinTheta * std::sin(azimuth),
                radius * cosTheta
            );
            Particle particle;
            particle.setMass(mass);
            particle.setPosition(centralPos + offset);
            particle.setVelocity(centralVel + Vector3(velDist(rng), velDist(rng), velDist(rng)));
            finalizeParticle(particle);
            meanPosition = meanPosition + particle.getPosition();
            meanVelocity = meanVelocity + particle.getVelocity();
            outParticles.push_back(particle);
        }

        const float invCount = 1.0f / static_cast<float>(outParticles.size());
        meanPosition = meanPosition * invCount;
        meanVelocity = meanVelocity * invCount;
        for (Particle &particle : outParticles) {
            particle.setPosition(particle.getPosition() - meanPosition + centralPos);
            particle.setVelocity(particle.getVelocity() - meanVelocity + centralVel);
        }
        return outParticles.size() >= 2;
    }

    if (mode == "random_cloud") {
        const float halfExtent = std::max(0.01f, config.cloudHalfExtent);
        const float cloudSpeed = std::max(0.0f, config.cloudSpeed);
        const float particleMass = std::max(1e-6f, config.particleMass);
        std::uniform_real_distribution<float> posDist(-halfExtent, halfExtent);
        std::uniform_real_distribution<float> velDist(-cloudSpeed, cloudSpeed);

        addCentralBody();
        while (outParticles.size() < count) {
            Particle p;
            p.setMass(particleMass);
            p.setPosition(Vector3(
                centralPos.x + posDist(rng),
                centralPos.y + posDist(rng),
                centralPos.z + posDist(rng)
            ));
            p.setVelocity(Vector3(
                centralVel.x + velDist(rng),
                centralVel.y + velDist(rng),
                centralVel.z + velDist(rng)
            ));
            finalizeParticle(p);
            outParticles.push_back(p);
        }
        return outParticles.size() >= 2;
    }

    // Default generated mode: disk_orbit.
    // Keep orbital initialization consistent with the force model:
    // solvers clamp acceleration magnitude to 64, so cap target orbital
    // acceleration accordingly to avoid injecting super-orbital velocities.
    constexpr float kSolverMaxAcceleration = 64.0f;
    const float radiusMin = std::max(0.01f, std::min(config.diskRadiusMin, config.diskRadiusMax));
    const float radiusMax = std::max(radiusMin + 1e-4f, std::max(config.diskRadiusMin, config.diskRadiusMax));
    const float radiusMin2 = radiusMin * radiusMin;
    const float radiusMax2 = radiusMax * radiusMax;
    const float radiusRange2 = std::max(1e-6f, radiusMax2 - radiusMin2);
    const float diskThickness = std::max(0.0f, config.diskThickness);
    const float velocityScale = std::max(0.0f, config.velocityScale);
    const float effectiveCentralMass = config.includeCentralBody ? centralMass : 0.0f;
    const float effectiveDiskMass = std::max(0.0f, config.diskMass);
    std::uniform_real_distribution<float> radiusDist(radiusMin, radiusMax);
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.1415926535f);
    std::uniform_real_distribution<float> zDist(-diskThickness, diskThickness);

    addCentralBody();
    const std::uint32_t diskCount = std::max<std::uint32_t>(1u, count - static_cast<std::uint32_t>(outParticles.size()));
    const float diskMassPerParticle = std::max(1e-6f, config.diskMass / static_cast<float>(diskCount));

    while (outParticles.size() < count) {
        const float r = radiusDist(rng);
        const float angle = angleDist(rng);
        const float z = zDist(rng);
        const Vector3 radial(r * std::cos(angle), r * std::sin(angle), z);
        const Vector3 position = centralPos + radial;

        const float enclosedFraction = std::clamp((r * r - radiusMin2) / radiusRange2, 0.0f, 1.0f);
        const float enclosedMass = std::max(1e-6f, effectiveCentralMass + effectiveDiskMass * enclosedFraction);
        const float gravityAccel = enclosedMass / std::max(r * r, 1e-6f);
        const float cappedAccel = std::min(gravityAccel, kSolverMaxAcceleration);
        const float orbitalSpeed = std::sqrt(cappedAccel * std::max(r, 1e-4f)) * velocityScale;
        Vector3 tangent(-std::sin(angle), std::cos(angle), 0.0f);
        tangent = tangent * orbitalSpeed;

        Particle p;
        p.setMass(diskMassPerParticle);
        p.setPosition(position);
        p.setVelocity(centralVel + tangent);
        finalizeParticle(p);
        outParticles.push_back(p);
    }

    return outParticles.size() >= 2;
}
SimulationServer::SimulationServer(std::uint32_t particleCount, float initialDt)
    : _running(false),
      _paused(false),
      _resetRequested(false),
      _exportRequested(false),
      _cudaContextDirty(false),
      _stepRequests(0),
      _dt(initialDt),
      _steps(0),
      _serverFps(0.0f),
      _kineticEnergy(0.0f),
      _potentialEnergy(0.0f),
      _thermalEnergy(0.0f),
      _radiatedEnergy(0.0f),
      _totalEnergy(0.0f),
      _energyDriftPct(0.0f),
      _energyEstimated(false),
      _energyMeasureEverySteps(30),
      _energySampleLimit(5000),
      _faulted(false),
      _faultStep(0),
      _particleCount(std::max<std::uint32_t>(2u, particleCount)),
      _solverMode("pairwise_cuda"),
      _integratorMode("euler"),
      _octreeTheta(1.2f),
      _octreeSoftening(2.5f),
      _sphEnabled(false),
      _sphSmoothingLength(1.25f),
      _sphRestDensity(1.0f),
      _sphGasConstant(4.0f),
      _sphViscosity(0.08f),
      _energyBaseline(0.0f),
      _hasEnergyBaseline(false),
      _pendingExportPath(),
      _pendingExportFormat("vtk"),
      _exportDirectory("exports"),
      _exportFormatDefault("vtk"),
      _initialStatePath(),
      _initialStateFormat("auto"),
      _configPath("simulation.ini"),
      _runtimeConfigMirror(),
      _initialStateConfig(),
      _thread(),
      _snapshotMutex(),
      _commandMutex(),
      _faultMutex(),
      _publishedSnapshot(),
      _scratchSnapshot(),
      _faultReason(),
      _system(nullptr)
{
    _runtimeConfigMirror.particleCount = _particleCount;
    _runtimeConfigMirror.dt = std::max(1e-6f, initialDt);
    _runtimeConfigMirror.solver = _solverMode;
    _runtimeConfigMirror.integrator = _integratorMode;
    _runtimeConfigMirror.octreeTheta = _octreeTheta;
    _runtimeConfigMirror.octreeSoftening = _octreeSoftening;
    _runtimeConfigMirror.sphEnabled = _sphEnabled;
    _runtimeConfigMirror.sphSmoothingLength = _sphSmoothingLength;
    _runtimeConfigMirror.sphRestDensity = _sphRestDensity;
    _runtimeConfigMirror.sphGasConstant = _sphGasConstant;
    _runtimeConfigMirror.sphViscosity = _sphViscosity;
    _runtimeConfigMirror.exportDirectory = _exportDirectory;
    _runtimeConfigMirror.exportFormat = _exportFormatDefault;
    _runtimeConfigMirror.inputFile = _initialStatePath;
    _runtimeConfigMirror.inputFormat = _initialStateFormat;
    _runtimeConfigMirror.energyMeasureEverySteps = _energyMeasureEverySteps.load(std::memory_order_relaxed);
    _runtimeConfigMirror.energySampleLimit = _energySampleLimit.load(std::memory_order_relaxed);
}

SimulationServer::SimulationServer(const std::string &configPath)
    : SimulationServer(2u, 0.01f)
{
    _configPath = configPath.empty() ? "simulation.ini" : configPath;
    const SimulationConfig loaded = SimulationConfig::loadOrCreate(_configPath);
    const ResolvedInitialStatePlan initPlan = resolveInitialStatePlan(loaded, std::cerr);
    _runtimeConfigMirror = loaded;

    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        _particleCount = std::max<std::uint32_t>(2u, loaded.particleCount);
        std::string solverCanonical;
        std::string integratorCanonical;
        _solverMode = grav_modes::normalizeSolver(loaded.solver, solverCanonical)
            ? solverCanonical
            : std::string(grav_modes::kSolverPairwiseCuda);
        _integratorMode = grav_modes::normalizeIntegrator(loaded.integrator, integratorCanonical)
            ? integratorCanonical
            : std::string(grav_modes::kIntegratorEuler);
        coerceConfigSolverIntegratorCompatibility(_solverMode, _integratorMode, "config");
        _octreeTheta = loaded.octreeTheta;
        _octreeSoftening = loaded.octreeSoftening;
        _sphEnabled = loaded.sphEnabled;
        _sphSmoothingLength = loaded.sphSmoothingLength;
        _sphRestDensity = loaded.sphRestDensity;
        _sphGasConstant = loaded.sphGasConstant;
        _sphViscosity = loaded.sphViscosity;
        _exportDirectory = loaded.exportDirectory;
        _exportFormatDefault = loaded.exportFormat;
        _initialStatePath = initPlan.inputFile;
        _initialStateFormat = initPlan.inputFormat;
        _initialStateConfig = initPlan.config;
    }
    std::cout << "[server] " << initPlan.summary << "\n";
    _dt.store(std::max(1e-6f, loaded.dt), std::memory_order_relaxed);
    _energyMeasureEverySteps.store(std::max<std::uint32_t>(1u, loaded.energyMeasureEverySteps), std::memory_order_relaxed);
    _energySampleLimit.store(std::max<std::uint32_t>(64u, loaded.energySampleLimit), std::memory_order_relaxed);
}

SimulationServer::~SimulationServer()
{
    stop();
    _system.reset();
}

void SimulationServer::start()
{
    if (_running.exchange(true)) {
        return;
    }
    std::cout << "[server] start particles=" << _particleCount
              << " dt=" << _dt.load(std::memory_order_relaxed) << "\n";
    _thread = std::thread(&SimulationServer::loop, this);
}

void SimulationServer::stop()
{
    if (!_running.exchange(false)) {
        return;
    }
    std::cout << "[server] stop requested\n";
    if (_thread.joinable()) {
        _thread.join();
    }
}

void SimulationServer::setPaused(bool paused)
{
    _paused.store(paused, std::memory_order_relaxed);
}

bool SimulationServer::isPaused() const
{
    return _paused.load(std::memory_order_relaxed);
}

void SimulationServer::togglePaused()
{
    _paused.store(!_paused.load(std::memory_order_relaxed), std::memory_order_relaxed);
}

void SimulationServer::stepOnce()
{
    _stepRequests.fetch_add(1, std::memory_order_relaxed);
}

void SimulationServer::setDt(float dt)
{
    if (dt > 0.0f) {
        _dt.store(dt, std::memory_order_relaxed);
    }
}

void SimulationServer::scaleDt(float factor)
{
    if (factor <= 0.0f) {
        return;
    }
    const float current = _dt.load(std::memory_order_relaxed);
    setDt(current * factor);
}

float SimulationServer::getDt() const
{
    return _dt.load(std::memory_order_relaxed);
}

void SimulationServer::requestReset()
{
    _stepRequests.store(0, std::memory_order_relaxed);
    _serverFps.store(0.0f, std::memory_order_relaxed);
    _paused.store(false, std::memory_order_relaxed);
    _resetRequested.store(true, std::memory_order_relaxed);
}

void SimulationServer::requestRecover()
{
    requestReset();
}

void SimulationServer::setParticleCount(std::uint32_t particleCount)
{
    const std::uint32_t clamped = std::max<std::uint32_t>(2u, particleCount);
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        _particleCount = clamped;
    }
    if (_running.load(std::memory_order_relaxed)) {
        requestReset();
    }
}

void SimulationServer::setSolverMode(const std::string &mode)
{
    std::string canonical;
    if (!grav_modes::normalizeSolver(mode, canonical)) {
        std::cerr << "[server] ignored invalid solver mode: " << mode << "\n";
        return;
    }
    bool changed = false;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        std::string nextSolver = canonical;
        if (!grav_modes::isSupportedSolverIntegratorPair(nextSolver, _integratorMode)) {
            std::cerr << "[server] rejected solver octree_gpu because integrator rk4 is not supported with it\n";
            return;
        }
        if (_solverMode != nextSolver) {
            _solverMode = nextSolver;
            changed = true;
            _runtimeConfigMirror.solver = _solverMode;
        }
    }
    if (changed && _running.load(std::memory_order_relaxed)) {
        requestReset();
    }
}

void SimulationServer::setIntegratorMode(const std::string &mode)
{
    std::string canonical;
    if (!grav_modes::normalizeIntegrator(mode, canonical)) {
        std::cerr << "[server] ignored invalid integrator mode: " << mode << "\n";
        return;
    }
    bool changed = false;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        std::string nextIntegrator = canonical;
        if (!grav_modes::isSupportedSolverIntegratorPair(_solverMode, nextIntegrator)) {
            std::cerr << "[server] rejected integrator rk4 because solver octree_gpu supports euler only\n";
            return;
        }
        if (_integratorMode != nextIntegrator) {
            _integratorMode = nextIntegrator;
            changed = true;
            _runtimeConfigMirror.integrator = _integratorMode;
        }
    }
    if (changed && _running.load(std::memory_order_relaxed)) {
        requestReset();
    }
}

void SimulationServer::setOctreeParameters(float theta, float softening)
{
    std::lock_guard<std::mutex> lock(_commandMutex);
    if (theta > 0.01f) {
        _octreeTheta = theta;
    }
    if (softening > 1e-6f) {
        _octreeSoftening = softening;
    }
}

void SimulationServer::setSphEnabled(bool enabled)
{
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        _sphEnabled = enabled;
    }
    if (_running.load(std::memory_order_relaxed)) {
        requestReset();
    }
}

void SimulationServer::setSphParameters(float smoothingLength, float restDensity, float gasConstant, float viscosity)
{
    std::lock_guard<std::mutex> lock(_commandMutex);
    if (smoothingLength > 0.05f) {
        _sphSmoothingLength = smoothingLength;
    }
    if (restDensity > 0.01f) {
        _sphRestDensity = restDensity;
    }
    if (gasConstant > 0.01f) {
        _sphGasConstant = gasConstant;
    }
    if (viscosity >= 0.0f) {
        _sphViscosity = viscosity;
    }
}

void SimulationServer::setInitialStateConfig(const InitialStateConfig &config)
{
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        _initialStateConfig = config;
    }
    if (_running.load(std::memory_order_relaxed)) {
        _resetRequested.store(true, std::memory_order_relaxed);
    }
}

void SimulationServer::setEnergyMeasurementConfig(std::uint32_t everySteps, std::uint32_t sampleLimit)
{
    _energyMeasureEverySteps.store(std::max<std::uint32_t>(1u, everySteps), std::memory_order_relaxed);
    _energySampleLimit.store(std::max<std::uint32_t>(64u, sampleLimit), std::memory_order_relaxed);
}

void SimulationServer::setExportDefaults(const std::string &directory, const std::string &format)
{
    std::lock_guard<std::mutex> lock(_commandMutex);
    if (!directory.empty()) {
        _exportDirectory = directory;
    }
    if (!format.empty()) {
        _exportFormatDefault = format;
    }
}

void SimulationServer::setInitialStateFile(const std::string &path, const std::string &format)
{
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        _initialStatePath = path;
        _initialStateFormat = format.empty() ? "auto" : format;
    }
    if (_running.load(std::memory_order_relaxed)) {
        _resetRequested.store(true, std::memory_order_relaxed);
    }
}

void SimulationServer::requestExportSnapshot(const std::string &outputPath, const std::string &format)
{
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        _pendingExportPath = outputPath;
        _pendingExportFormat = format.empty() ? _exportFormatDefault : format;
    }
    _exportRequested.store(true, std::memory_order_relaxed);
}

bool SimulationServer::tryConsumeSnapshot(std::vector<RenderParticle> &outSnapshot)
{
    std::lock_guard<std::mutex> lock(_snapshotMutex);
    if (_publishedSnapshot.empty()) {
        return false;
    }
    outSnapshot = std::move(_publishedSnapshot);
    _publishedSnapshot.clear();
    return true;
}

bool SimulationServer::copyLatestSnapshot(std::vector<RenderParticle> &outSnapshot, std::size_t maxPoints) const
{
    std::lock_guard<std::mutex> lock(_snapshotMutex);
    if (_publishedSnapshot.empty()) {
        outSnapshot.clear();
        return false;
    }
    if (maxPoints == 0 || _publishedSnapshot.size() <= maxPoints) {
        outSnapshot = _publishedSnapshot;
        return true;
    }

    outSnapshot.clear();
    outSnapshot.reserve(maxPoints);
    const std::size_t stride = std::max<std::size_t>(1, (_publishedSnapshot.size() + maxPoints - 1u) / maxPoints);
    for (std::size_t i = 0; i < _publishedSnapshot.size() && outSnapshot.size() < maxPoints; i += stride) {
        outSnapshot.push_back(_publishedSnapshot[i]);
    }
    return true;
}

SimulationStats SimulationServer::getStats() const
{
    ParticleSystem::SolverMode mode = ParticleSystem::SolverMode::PairwiseCuda;
    std::string integratorMode;
    std::uint32_t particleCount = 0u;
    bool sphEnabled = false;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        mode = solverModeFromCanonicalName(_solverMode);
        integratorMode = _integratorMode;
        particleCount = _particleCount;
        sphEnabled = _sphEnabled;
    }
    std::string faultReason;
    {
        std::lock_guard<std::mutex> lock(_faultMutex);
        faultReason = _faultReason;
    }
    return SimulationStats{
        _steps.load(std::memory_order_relaxed),
        _dt.load(std::memory_order_relaxed),
        _paused.load(std::memory_order_relaxed),
        _faulted.load(std::memory_order_relaxed),
        _faultStep.load(std::memory_order_relaxed),
        std::move(faultReason),
        sphEnabled,
        _serverFps.load(std::memory_order_relaxed),
        particleCount,
        _kineticEnergy.load(std::memory_order_relaxed),
        _potentialEnergy.load(std::memory_order_relaxed),
        _thermalEnergy.load(std::memory_order_relaxed),
        _radiatedEnergy.load(std::memory_order_relaxed),
        _totalEnergy.load(std::memory_order_relaxed),
        _energyDriftPct.load(std::memory_order_relaxed),
        _energyEstimated.load(std::memory_order_relaxed),
        std::string(solverLabel(mode)),
        integratorMode
    };
}

SimulationConfig SimulationServer::getRuntimeConfig() const
{
    SimulationConfig config;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        config = _runtimeConfigMirror;
        config.particleCount = _particleCount;
        config.solver = _solverMode;
        config.integrator = _integratorMode;
        config.octreeTheta = _octreeTheta;
        config.octreeSoftening = _octreeSoftening;
        config.sphEnabled = _sphEnabled;
        config.sphSmoothingLength = _sphSmoothingLength;
        config.sphRestDensity = _sphRestDensity;
        config.sphGasConstant = _sphGasConstant;
        config.sphViscosity = _sphViscosity;
        config.exportDirectory = _exportDirectory;
        config.exportFormat = _exportFormatDefault;
        config.inputFile = _initialStatePath;
        config.inputFormat = _initialStateFormat;
    }
    config.dt = _dt.load(std::memory_order_relaxed);
    config.energyMeasureEverySteps = _energyMeasureEverySteps.load(std::memory_order_relaxed);
    config.energySampleLimit = _energySampleLimit.load(std::memory_order_relaxed);
    return config;
}

bool SimulationServer::loadInitialState(std::vector<Particle> &outParticles, const std::string &inputPath, const std::string &format) const
{
    outParticles.clear();
    if (inputPath.empty()) {
        return false;
    }

    const std::filesystem::path path(inputPath);
    if (!std::filesystem::exists(path)) {
        std::cerr << "[server] input file not found: " << inputPath << "\n";
        return false;
    }

    std::string fmt = normalizeSnapshotFormat(format.empty() ? std::string("auto") : format);
    if (fmt == "auto") {
        fmt = normalizeSnapshotFormat(guessFormatFromPath(inputPath));
    }
    if (fmt == "vtk_binary") {
        fmt = "vtk";
    }

    bool loaded = false;
    if (fmt == "bin") {
        loaded = parseBinarySnapshot(inputPath, outParticles);
    } else if (fmt == "vtk") {
        loaded = parseVtkSnapshot(inputPath, outParticles);
    } else if (fmt == "xyz") {
        loaded = parseXyzSnapshot(inputPath, outParticles);
    } else {
        loaded = parseBinarySnapshot(inputPath, outParticles);
        if (!loaded) {
            loaded = parseVtkSnapshot(inputPath, outParticles);
        }
        if (!loaded) {
            loaded = parseXyzSnapshot(inputPath, outParticles);
        }
    }

    if (!loaded || outParticles.size() < 2) {
        std::cerr << "[server] failed to parse input state: " << inputPath
                  << " (format=" << (format.empty() ? "auto" : format) << ")\n";
        outParticles.clear();
        return false;
    }
    return true;
}

void SimulationServer::rebuildSystem()
{
    // Important with the current CUDA global-buffer model:
    // destroy the previous ParticleSystem before constructing the next one,
    // otherwise the old destructor may free buffers allocated by the new instance.
    _system.reset();

    std::string solver;
    std::string integrator;
    float theta = 1.2f;
    float softening = 2.5f;
    bool sphEnabled = false;
    float sphSmoothingLength = 1.25f;
    float sphRestDensity = 1.0f;
    float sphGasConstant = 4.0f;
    float sphViscosity = 0.08f;
    std::string inputPath;
    std::string inputFormat;
    InitialStateConfig initConfig;
    std::uint32_t configuredParticleCount = 2u;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        solver = _solverMode;
        integrator = _integratorMode;
        theta = _octreeTheta;
        softening = _octreeSoftening;
        sphEnabled = _sphEnabled;
        sphSmoothingLength = _sphSmoothingLength;
        sphRestDensity = _sphRestDensity;
        sphGasConstant = _sphGasConstant;
        sphViscosity = _sphViscosity;
        inputPath = _initialStatePath;
        inputFormat = _initialStateFormat;
        initConfig = _initialStateConfig;
        configuredParticleCount = std::max<std::uint32_t>(2u, _particleCount);
    }

    {
        std::string canonical;
        if (grav_modes::normalizeSolver(solver, canonical)) {
            solver = canonical;
        } else {
            solver.assign(grav_modes::kSolverPairwiseCuda);
            std::cerr << "[server] invalid internal solver mode detected, resetting to pairwise_cuda\n";
        }
        if (grav_modes::normalizeIntegrator(integrator, canonical)) {
            integrator = canonical;
        } else {
            integrator.assign(grav_modes::kIntegratorEuler);
            std::cerr << "[server] invalid internal integrator mode detected, resetting to euler\n";
        }
        coerceConfigSolverIntegratorCompatibility(solver, integrator, "rebuild");
    }

    std::vector<Particle> importedParticles;
    const std::string initMode = toLower(initConfig.mode);
    const bool shouldTryFile = initMode == "file" && !inputPath.empty();
    const bool hasImportedState = shouldTryFile && loadInitialState(importedParticles, inputPath, inputFormat);

    std::vector<Particle> generatedParticles;
    const bool hasGeneratedState = hasImportedState
        ? false
        : buildGeneratedState(generatedParticles, configuredParticleCount, initConfig);

    const std::vector<Particle> &initialParticles = hasImportedState ? importedParticles : generatedParticles;
    const bool hasInitialParticles = !initialParticles.empty();
    const std::uint32_t targetParticleCount = hasInitialParticles
        ? static_cast<std::uint32_t>(std::max<std::size_t>(2u, initialParticles.size()))
        : configuredParticleCount;
    std::cout << "[server] init rebuild mode=" << initMode
              << " active_source=" << (hasImportedState ? "file" : "generated");
    if (shouldTryFile) {
        std::cout << " input_file=" << inputPath
                  << " input_format=" << (inputFormat.empty() ? "auto" : inputFormat);
    }
    std::cout << " particles=" << targetParticleCount << "\n";

    std::string effectiveSolver = solver;
    if (solverModeFromCanonicalName(solver) == ParticleSystem::SolverMode::PairwiseCuda
        && targetParticleCount > kPairwiseRealtimeParticleLimit) {
        if (isAutoSolverFallbackEnabled()) {
            effectiveSolver = "octree_gpu";
            std::cout << "[server] pairwise_cuda with " << targetParticleCount
                      << " particles is not realtime; auto-switching to octree_gpu\n";
        } else {
            std::cout << "[server] warning: pairwise_cuda with " << targetParticleCount
                      << " particles may look frozen; set solver=octree_gpu"
                      << " or GRAVITY_AUTO_SOLVER_FALLBACK=1\n";
        }
    }
    coerceConfigSolverIntegratorCompatibility(effectiveSolver, integrator, "rebuild");
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        _solverMode = effectiveSolver;
        _integratorMode = integrator;
    }

    if (hasInitialParticles) {
        std::vector<Particle> particles = initialParticles;
        for (Particle &p : particles) {
            p.setPressure(Vector3(0.0f, 0.0f, 0.0f));
            p.setDensity(0.0f);
            float temp = p.getTemperature();
            if (temp < 0.0f) {
                temp = 0.0f;
            }
            if (temp == 0.0f && initConfig.particleTemperature > 0.0f) {
                temp = initConfig.particleTemperature;
            }
            p.setTemperature(temp);
        }
        _system = std::make_unique<ParticleSystem>(std::move(particles));
    } else {
        _system = std::make_unique<ParticleSystem>(static_cast<int>(targetParticleCount), false);
    }

    const std::vector<Particle> &configuredParticles = _system->getParticles();
    if (hasImportedState) {
        std::cout << "[server] loaded initial state from " << inputPath
                  << " particles=" << configuredParticles.size() << "\n";
    } else if (hasGeneratedState) {
        std::cout << "[server] generated initial state mode=" << initConfig.mode
                  << " particles=" << configuredParticles.size() << "\n";
    } else {
        std::cerr << "[server] initial state generation failed, using constructor fallback\n";
    }

    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        _particleCount = targetParticleCount;
    }

    _system->setOctreeTheta(theta);
    _system->setOctreeSoftening(softening);
    _system->setSolverMode(solverModeFromCanonicalName(effectiveSolver));
    _system->setIntegratorMode(integratorModeFromCanonicalName(integrator));
    _system->setSphEnabled(sphEnabled);
    _system->setSphParameters(sphSmoothingLength, sphRestDensity, sphGasConstant, sphViscosity);
    _system->setThermalParameters(
        initConfig.thermalAmbientTemperature,
        initConfig.thermalSpecificHeat,
        initConfig.thermalHeatingCoeff,
        initConfig.thermalRadiationCoeff
    );
    logEffectiveExecutionModes(effectiveSolver, integrator, theta, softening, sphEnabled);
    if (initConfig.thermalHeatingCoeff > 0.0f || initConfig.thermalRadiationCoeff > 0.0f) {
        std::cout << "[server] warning: thermal model active (heating="
                  << initConfig.thermalHeatingCoeff
                  << ", radiation=" << initConfig.thermalRadiationCoeff
                  << ") may change total energy drift\n";
    }

    _steps.store(0, std::memory_order_relaxed);
    _serverFps.store(0.0f, std::memory_order_relaxed);
    _stepRequests.store(0, std::memory_order_relaxed);
    _kineticEnergy.store(0.0f, std::memory_order_relaxed);
    _potentialEnergy.store(0.0f, std::memory_order_relaxed);
    _thermalEnergy.store(0.0f, std::memory_order_relaxed);
    _radiatedEnergy.store(0.0f, std::memory_order_relaxed);
    _totalEnergy.store(0.0f, std::memory_order_relaxed);
    _energyDriftPct.store(0.0f, std::memory_order_relaxed);
    _energyEstimated.store(false, std::memory_order_relaxed);
    _hasEnergyBaseline = false;
    _energyBaseline = 0.0f;
    _faulted.store(false, std::memory_order_relaxed);
    _faultStep.store(0, std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lock(_faultMutex);
        _faultReason.clear();
    }

    _scratchSnapshot.clear();
    publishSnapshot();
    maybeUpdateEnergy(0);
}

void SimulationServer::publishSnapshot()
{
    if (!_system) {
        return;
    }
    if (!_system->syncHostState()) {
        return;
    }
    const std::vector<Particle> &particles = _system->getParticles();
    const size_t count = particles.size();
    if (_scratchSnapshot.size() != count) {
        _scratchSnapshot.resize(count);
    }
    for (size_t i = 0; i < count; ++i) {
        _scratchSnapshot[i] = RenderParticle{
            particles[i].getPosition().x,
            particles[i].getPosition().y,
            particles[i].getPosition().z,
            particles[i].getMass(),
            particles[i].getPressure().norm(),
            particles[i].getTemperature()
        };
    }

    std::lock_guard<std::mutex> lock(_snapshotMutex);
    _publishedSnapshot.swap(_scratchSnapshot);
}

SimulationServer::EnergyValues SimulationServer::computeEnergyValues()
{
    EnergyValues values{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, false};
    if (!_system) {
        return values;
    }
    if (!_system->syncHostState()) {
        values.estimated = true;
        return values;
    }
    const std::vector<Particle> &particles = _system->getParticles();
    const std::size_t n = particles.size();
    if (n < 2) {
        return values;
    }

    const std::size_t sampleLimit = static_cast<std::size_t>(_energySampleLimit.load(std::memory_order_relaxed));
    const bool sampled = n > sampleLimit;
    const float specificHeat = _system ? std::max(1e-6f, _system->getThermalSpecificHeat()) : 1.0f;
    float energySoftening = 0.05f;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        energySoftening = std::max(1e-4f, _octreeSoftening);
    }
    std::vector<std::size_t> indices;
    if (!sampled) {
        indices.resize(n);
        for (std::size_t i = 0; i < n; ++i) {
            indices[i] = i;
        }
    } else {
        const std::size_t sampleCount = std::max<std::size_t>(64u, sampleLimit);
        const std::size_t stride = std::max<std::size_t>(1, n / sampleCount);
        for (std::size_t i = 0; i < n; i += stride) {
            indices.push_back(i);
            if (indices.size() >= sampleCount) {
                break;
            }
        }
    }

    const double kineticScale = sampled ? static_cast<double>(n) / static_cast<double>(indices.size()) : 1.0;
    const double pairCountFull = static_cast<double>(n) * static_cast<double>(n - 1) * 0.5;
    const double pairCountSample = static_cast<double>(indices.size()) * static_cast<double>(indices.size() - 1) * 0.5;
    const double potentialScale = (sampled && pairCountSample > 0.0) ? (pairCountFull / pairCountSample) : 1.0;
    const float softening = energySoftening;

    double kinetic = 0.0;
    double thermal = 0.0;
    for (std::size_t idx : indices) {
        const Particle &p = particles[idx];
        const Vector3 v = p.getVelocity();
        const double speed2 = static_cast<double>(v.x * v.x + v.y * v.y + v.z * v.z);
        kinetic += 0.5 * static_cast<double>(p.getMass()) * speed2;
        thermal += static_cast<double>(p.getMass()) * static_cast<double>(specificHeat) * static_cast<double>(std::max(0.0f, p.getTemperature()));
    }
    kinetic *= kineticScale;
    thermal *= kineticScale;

    double potential = 0.0;
    for (std::size_t a = 0; a < indices.size(); ++a) {
        const Particle &p = particles[indices[a]];
        const Vector3 pp = p.getPosition();
        for (std::size_t b = a + 1; b < indices.size(); ++b) {
            const Particle &q = particles[indices[b]];
            const Vector3 qq = q.getPosition();
            const float dx = pp.x - qq.x;
            const float dy = pp.y - qq.y;
            const float dz = pp.z - qq.z;
            const float dist = std::sqrt(dx * dx + dy * dy + dz * dz + softening * softening);
            if (dist <= 1e-6f) {
                continue;
            }
            potential -= static_cast<double>(p.getMass()) * static_cast<double>(q.getMass()) / static_cast<double>(dist);
        }
    }
    potential *= potentialScale;

    values.kinetic = static_cast<float>(kinetic);
    values.potential = static_cast<float>(potential);
    values.thermal = static_cast<float>(thermal);
    values.radiated = _system ? _system->getCumulativeRadiatedEnergy() : 0.0f;
    values.total = values.kinetic + values.potential + values.thermal + values.radiated;
    values.estimated = sampled;
    return values;
}

void SimulationServer::maybeUpdateEnergy(std::uint64_t currentStep)
{
    const std::uint32_t every = _energyMeasureEverySteps.load(std::memory_order_relaxed);
    if (every == 0 || (currentStep % every) != 0) {
        return;
    }

    const EnergyValues values = computeEnergyValues();
    _kineticEnergy.store(values.kinetic, std::memory_order_relaxed);
    _potentialEnergy.store(values.potential, std::memory_order_relaxed);
    _thermalEnergy.store(values.thermal, std::memory_order_relaxed);
    _radiatedEnergy.store(values.radiated, std::memory_order_relaxed);
    _totalEnergy.store(values.total, std::memory_order_relaxed);
    _energyEstimated.store(values.estimated, std::memory_order_relaxed);

    if (!_hasEnergyBaseline) {
        _energyBaseline = values.total;
        _hasEnergyBaseline = true;
        _energyDriftPct.store(0.0f, std::memory_order_relaxed);
        return;
    }

    const float denom = std::max(std::fabs(_energyBaseline), 1e-6f);
    const float drift = ((values.total - _energyBaseline) / denom) * 100.0f;
    _energyDriftPct.store(drift, std::memory_order_relaxed);
}

void SimulationServer::processPendingExport()
{
    if (!_exportRequested.exchange(false, std::memory_order_relaxed)) {
        return;
    }
    std::string outputPath;
    std::string format;
    std::string exportDirectory;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        outputPath = _pendingExportPath;
        format = _pendingExportFormat.empty() ? _exportFormatDefault : _pendingExportFormat;
        exportDirectory = _exportDirectory;
    }
    if (outputPath.empty()) {
        outputPath = defaultExportPath(exportDirectory, format, _steps.load(std::memory_order_relaxed));
    }
    if (exportCurrentState(outputPath, format)) {
        std::cout << "[server] export ok: " << outputPath << "\n";
    } else {
        std::cerr << "[server] export failed: " << outputPath << "\n";
    }
}

bool SimulationServer::exportCurrentState(const std::string &outputPath, const std::string &format)
{
    if (!_system) {
        return false;
    }
    if (!_system->syncHostState()) {
        return false;
    }
    const std::vector<Particle> &particles = _system->getParticles();
    std::string solverModeLabel;
    std::string integratorModeLabel;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        solverModeLabel = _solverMode;
        integratorModeLabel = _integratorMode;
    }
    std::filesystem::path outPath(outputPath);
    if (outPath.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(outPath.parent_path(), ec);
    }

    std::string fmt = normalizeSnapshotFormat(format);

    if (fmt == "bin") {
        std::ofstream out(outputPath, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            return false;
        }
        BinarySnapshotHeader header{};
        std::memcpy(header.magic, kBinarySnapshotMagic, sizeof(kBinarySnapshotMagic));
        header.version = kBinarySnapshotVersion;
        header.count = static_cast<std::uint32_t>(particles.size());
        std::array<std::byte, sizeof(BinarySnapshotHeader)> headerBytes{};
        std::memcpy(headerBytes.data(), &header, sizeof(header));
        if (!writeRawBytes(out, headerBytes.data(), headerBytes.size())) {
            return false;
        }
        for (const Particle &p : particles) {
            const Vector3 pos = p.getPosition();
            const Vector3 vel = p.getVelocity();
            const BinarySnapshotParticle rec{
                pos.x,
                pos.y,
                pos.z,
                vel.x,
                vel.y,
                vel.z,
                p.getMass(),
                p.getTemperature()
            };
            std::array<std::byte, sizeof(BinarySnapshotParticle)> recBytes{};
            std::memcpy(recBytes.data(), &rec, sizeof(rec));
            if (!writeRawBytes(out, recBytes.data(), recBytes.size())) {
                return false;
            }
        }
        return true;
    }

    const bool vtkBinary = (fmt == "vtk_binary");
    std::ofstream out(
        outputPath,
        (vtkBinary ? (std::ios::binary | std::ios::trunc) : std::ios::trunc)
    );
    if (!out.is_open()) {
        return false;
    }

    if (fmt == "xyz") {
        out << particles.size() << "\n";
        out << "solver=" << solverModeLabel
            << " integrator=" << integratorModeLabel
            << " step=" << _steps.load(std::memory_order_relaxed) << "\n";
        for (const Particle &p : particles) {
            const Vector3 pos = p.getPosition();
            out << "P " << pos.x << " " << pos.y << " " << pos.z << " " << p.getMass() << " " << p.getTemperature() << "\n";
        }
        return true;
    }

    out << "# vtk DataFile Version 3.0\n";
    out << "CUDA gravity snapshot solver=" << solverModeLabel << " integrator=" << integratorModeLabel << "\n";
    out << (vtkBinary ? "BINARY\n" : "ASCII\n");
    out << "DATASET POLYDATA\n";
    out << "POINTS " << particles.size() << " float\n";
    if (vtkBinary) {
        for (const Particle &p : particles) {
            const Vector3 pos = p.getPosition();
            writeBeF32(out, pos.x);
            writeBeF32(out, pos.y);
            writeBeF32(out, pos.z);
        }
        out << "\n";
    } else {
        for (const Particle &p : particles) {
            const Vector3 pos = p.getPosition();
            out << pos.x << " " << pos.y << " " << pos.z << "\n";
        }
    }

    out << "VERTICES " << particles.size() << " " << (particles.size() * 2) << "\n";
    if (vtkBinary) {
        for (std::size_t i = 0; i < particles.size(); ++i) {
            writeBeI32(out, 1);
            writeBeI32(out, static_cast<std::int32_t>(i));
        }
        out << "\n";
    } else {
        for (std::size_t i = 0; i < particles.size(); ++i) {
            out << "1 " << i << "\n";
        }
    }

    out << "POINT_DATA " << particles.size() << "\n";
    out << "SCALARS mass float 1\n";
    out << "LOOKUP_TABLE default\n";
    if (vtkBinary) {
        for (const Particle &p : particles) {
            writeBeF32(out, p.getMass());
        }
        out << "\n";
    } else {
        for (const Particle &p : particles) {
            out << p.getMass() << "\n";
        }
    }

    out << "SCALARS pressure float 1\n";
    out << "LOOKUP_TABLE default\n";
    if (vtkBinary) {
        for (const Particle &p : particles) {
            writeBeF32(out, p.getPressure().norm());
        }
        out << "\n";
    } else {
        for (const Particle &p : particles) {
            out << p.getPressure().norm() << "\n";
        }
    }

    out << "SCALARS temperature float 1\n";
    out << "LOOKUP_TABLE default\n";
    if (vtkBinary) {
        for (const Particle &p : particles) {
            writeBeF32(out, p.getTemperature());
        }
        out << "\n";
    } else {
        for (const Particle &p : particles) {
            out << p.getTemperature() << "\n";
        }
    }

    out << "VECTORS velocity float\n";
    if (vtkBinary) {
        for (const Particle &p : particles) {
            const Vector3 v = p.getVelocity();
            writeBeF32(out, v.x);
            writeBeF32(out, v.y);
            writeBeF32(out, v.z);
        }
        out << "\n";
    } else {
        for (const Particle &p : particles) {
            const Vector3 v = p.getVelocity();
            out << v.x << " " << v.y << " " << v.z << "\n";
        }
    }
    return true;
}

void SimulationServer::loop()
{
    rebuildSystem();
    auto nextSnapshotPublish = std::chrono::steady_clock::now();
    constexpr auto kSnapshotPublishPeriod = std::chrono::milliseconds(16);

    while (_running.load(std::memory_order_relaxed)) {
        if (_resetRequested.exchange(false, std::memory_order_relaxed)) {
            if (_cudaContextDirty.exchange(false, std::memory_order_relaxed)) {
                _system.reset();
                const cudaError_t resetStatus = cudaDeviceReset();
                if (resetStatus != cudaSuccess) {
                    std::cerr << "[server] cudaDeviceReset failed: "
                              << cudaGetErrorString(resetStatus) << "\n";
                } else {
                    std::cout << "[server] CUDA context reset after previous failure\n";
                }
            }
            rebuildSystem();
        }

        std::uint32_t stepBatch = 0;
        if (_paused.load(std::memory_order_relaxed)) {
            stepBatch = _stepRequests.exchange(0, std::memory_order_relaxed);
            if (stepBatch == 0) {
                processPendingExport();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
        } else {
            stepBatch = 1;
            _stepRequests.store(0, std::memory_order_relaxed);
        }

        float theta = 1.2f;
        float softening = 2.5f;
        float sphSmoothingLength = 1.25f;
        float sphRestDensity = 1.0f;
        float sphGasConstant = 4.0f;
        float sphViscosity = 0.08f;
        std::string solverMode;
        std::string integratorMode;
        bool sphEnabled = false;
        {
            std::lock_guard<std::mutex> lock(_commandMutex);
            theta = _octreeTheta;
            softening = _octreeSoftening;
            solverMode = _solverMode;
            integratorMode = _integratorMode;
            sphEnabled = _sphEnabled;
            sphSmoothingLength = _sphSmoothingLength;
            sphRestDensity = _sphRestDensity;
            sphGasConstant = _sphGasConstant;
            sphViscosity = _sphViscosity;
        }
        _system->setSolverMode(solverModeFromCanonicalName(solverMode));
        _system->setIntegratorMode(integratorModeFromCanonicalName(integratorMode));
        _system->setOctreeTheta(theta);
        _system->setOctreeSoftening(softening);
        _system->setSphEnabled(sphEnabled);
        _system->setSphParameters(sphSmoothingLength, sphRestDensity, sphGasConstant, sphViscosity);

        const auto batchStart = std::chrono::steady_clock::now();
        std::uint32_t executedSteps = 0;
        bool updateFailed = false;
        const bool steppedWhilePaused = _paused.load(std::memory_order_relaxed) && stepBatch > 0;
        for (std::uint32_t i = 0; i < stepBatch; ++i) {
            if (!_running.load(std::memory_order_relaxed) || _resetRequested.load(std::memory_order_relaxed)) {
                break;
            }
            if (shouldForceCudaFailureOnceForTesting(solverMode)) {
                std::cerr << "[server] forcing CUDA failure once for integration test\n";
                updateFailed = true;
                break;
            }
            const float dt = std::max(1e-6f, _dt.load(std::memory_order_relaxed));
            const std::size_t liveParticleCount = _system ? _system->getParticles().size() : 0u;
            const bool eulerIntegrator = _system->getIntegratorMode() == ParticleSystem::IntegratorMode::Euler;
            const float targetSubstep = sphEnabled ? 0.001f : (eulerIntegrator ? 0.0002f : 0.0025f);
            float adjustedTargetSubstep = targetSubstep;
            if (!sphEnabled) {
                if (liveParticleCount >= 100'000u) {
                    adjustedTargetSubstep = std::max(adjustedTargetSubstep, 0.01f);
                } else if (liveParticleCount >= 20'000u) {
                    adjustedTargetSubstep = std::max(adjustedTargetSubstep, 0.005f);
                }
            }
            std::uint32_t substeps = static_cast<std::uint32_t>(std::ceil(dt / adjustedTargetSubstep));
            substeps = std::max<std::uint32_t>(1u, std::min<std::uint32_t>(substeps, 256u));
            const float dtSub = dt / static_cast<float>(substeps);
            for (std::uint32_t s = 0; s < substeps; ++s) {
                if (!_system->update(dtSub)) {
                    updateFailed = true;
                    break;
                }
            }
            if (updateFailed) {
                break;
            }
            _steps.fetch_add(1, std::memory_order_relaxed);
            ++executedSteps;
        }
        const auto batchEnd = std::chrono::steady_clock::now();
        const std::chrono::duration<float> elapsed = batchEnd - batchStart;
        if (executedSteps == 0 || elapsed.count() <= 1e-6f) {
            _serverFps.store(0.0f, std::memory_order_relaxed);
        } else {
            const float instantStepsPerSecond = static_cast<float>(executedSteps) / elapsed.count();
            const float previous = _serverFps.load(std::memory_order_relaxed);
            const float smoothed = (previous <= 0.0f)
                ? instantStepsPerSecond
                : (0.85f * previous + 0.15f * instantStepsPerSecond);
            _serverFps.store(smoothed, std::memory_order_relaxed);
        }
        if (updateFailed) {
            bool autoFallbackToCpu = false;
            std::string previousSolver;
            {
                std::lock_guard<std::mutex> lock(_commandMutex);
                previousSolver = _solverMode;
                if (_solverMode == "pairwise_cuda" || _solverMode == "octree_gpu") {
                    _solverMode = "octree_cpu";
                    autoFallbackToCpu = true;
                }
            }
            if (autoFallbackToCpu) {
                _serverFps.store(0.0f, std::memory_order_relaxed);
                _cudaContextDirty.store(true, std::memory_order_relaxed);
                requestReset();
                _faulted.store(false, std::memory_order_relaxed);
                _faultStep.store(0, std::memory_order_relaxed);
                {
                    std::lock_guard<std::mutex> lock(_faultMutex);
                    _faultReason.clear();
                }
                std::cerr << "[server] CUDA update failed with solver=" << previousSolver
                          << ", auto-fallback to solver=octree_cpu\n";
            } else {
                _serverFps.store(0.0f, std::memory_order_relaxed);
                _paused.store(true, std::memory_order_relaxed);
                _cudaContextDirty.store(true, std::memory_order_relaxed);
                _faulted.store(true, std::memory_order_relaxed);
                _faultStep.store(_steps.load(std::memory_order_relaxed), std::memory_order_relaxed);
                {
                    std::lock_guard<std::mutex> lock(_faultMutex);
                    _faultReason = "cuda update failed (request recover/reset)";
                }
                std::cerr << "[server] update failed (CUDA error), simulation paused\n";
            }
        }

        const auto now = std::chrono::steady_clock::now();
        const bool publishByCadence = (executedSteps > 0) && (now >= nextSnapshotPublish);
        const bool publishAfterStepRequest = steppedWhilePaused && executedSteps > 0;
        if (publishByCadence || publishAfterStepRequest || updateFailed) {
            publishSnapshot();
            nextSnapshotPublish = now + kSnapshotPublishPeriod;
        }
        maybeUpdateEnergy(_steps.load(std::memory_order_relaxed));
        processPendingExport();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
