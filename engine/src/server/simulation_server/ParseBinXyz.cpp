// File: engine/src/server/simulation_server/ParseBinXyz.cpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#include "Internal.hpp"

/// Description: Executes the parseBinarySnapshot operation.
bool parseBinarySnapshot(const std::string& inputPath, std::vector<Particle>& outParticles)
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
    if (fileSize < 0)
        return false;
    const std::size_t expectedSize =
        sizeof(BinarySnapshotHeader) +
        static_cast<std::size_t>(header.count) * sizeof(BinarySnapshotParticle);
    if (static_cast<std::size_t>(fileSize) < expectedSize)
        return false;
    in.seekg(static_cast<std::streamoff>(sizeof(BinarySnapshotHeader)), std::ios::beg);
    if (!in)
        return false;
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

/// Description: Executes the parseXyzSnapshot operation.
bool parseXyzSnapshot(const std::string& inputPath, std::vector<Particle>& outParticles)
{
    std::ifstream in(inputPath);
    if (!in.is_open())
        return false;
    std::size_t expectedCount = 0;
    {
        std::string firstLine;
        if (!std::getline(in, firstLine))
            return false;
        std::istringstream iss(firstLine);
        iss >> expectedCount;
    }
    if (expectedCount > kMaxImportedParticles)
        return false;
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
        auto tryParseFloat = [](const std::string& value, float& out) {
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
        if (!tryParseFloat(tokens[offset + 0], x) || !tryParseFloat(tokens[offset + 1], y) ||
            !tryParseFloat(tokens[offset + 2], z)) {
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

/// Description: Describes the parse snapshot by format operation contract.
bool parseSnapshotByFormat(std::string_view format, const std::string& inputPath,
                           std::vector<Particle>& outParticles)
{
    struct FormatParserEntry {
        std::string_view format;
        bool (*parser)(const std::string&, std::vector<Particle>&);
    };

    static const std::array<FormatParserEntry, 3> parsers = {
        {{"bin", parseBinarySnapshot}, {"vtk", parseVtkSnapshot}, {"xyz", parseXyzSnapshot}}};
    for (const FormatParserEntry& entry : parsers)
        if (format == entry.format)
            return entry.parser(inputPath, outParticles);
    return false;
}

/// Description: Executes the parseSnapshotWithFallback operation.
bool parseSnapshotWithFallback(const std::string& inputPath, std::vector<Particle>& outParticles)
{
    static const std::array<std::string_view, 3> fallbackOrder = {{"bin", "vtk", "xyz"}};
    for (std::string_view format : fallbackOrder)
        if (parseSnapshotByFormat(format, inputPath, outParticles)) {
            return true;
        }
    return false;
}
