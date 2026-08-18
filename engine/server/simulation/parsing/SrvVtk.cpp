/*
 * @file engine/server/simulation/parsing/SrvVtk.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief VTK snapshot import for ASCII and big-endian binary payloads.
 */

#include "server/simulation/runtime/SrvInternal.hpp"

namespace blitzar_simulation_vtk {
struct VtkBuffers {
    std::vector<Vector3> positions;
    std::vector<float> masses;
    std::vector<float> temperatures;
    std::vector<Vector3> velocities;
};

bool finalizeParticles(const VtkBuffers& buffers, std::vector<Particle>& outParticles)
{
    if (!isValidImportedParticleCount(buffers.positions.size())) {
        return false;
    }
    outParticles.resize(buffers.positions.size());
    for (std::size_t index = 0; index < buffers.positions.size(); ++index) {
        Particle particle;
        particle.setPosition(buffers.positions[index]);
        particle.setVelocity(index < buffers.velocities.size()
                                 ? buffers.velocities[index]
                                 : Vector3(0.0f, 0.0f, 0.0f));
        particle.setPressure(Vector3(0.0f, 0.0f, 0.0f));
        particle.setDensity(0.0f);
        particle.setTemperature(0.0f);
        if (index < buffers.masses.size() && buffers.masses[index] > 0.0f) {
            particle.setMass(buffers.masses[index]);
        }
        if (index < buffers.temperatures.size()) {
            particle.setTemperature(std::max(0.0f, buffers.temperatures[index]));
        }
        outParticles[index] = particle;
    }
    return true;
}

bool readAsciiScalars(std::istream& input, std::size_t pointCount, const std::string& name,
                      int components, VtkBuffers& buffers)
{
    std::string lookup;
    std::string tableName;
    input >> lookup >> tableName;
    const std::size_t scalarCount =
        pointCount * static_cast<std::size_t>(std::max(1, components));
    const std::string normalizedName = toLower(name);
    if ((normalizedName == "mass" || normalizedName == "temperature") && components == 1) {
        std::vector<float>& target = normalizedName == "mass" ? buffers.masses : buffers.temperatures;
        target.resize(pointCount);
        for (float& value : target) {
            if (!(input >> value)) {
                return false;
            }
        }
        return true;
    }
    float discarded = 0.0f;
    for (std::size_t index = 0; index < scalarCount; ++index) {
        if (!(input >> discarded)) {
            return false;
        }
    }
    return true;
}

bool readAsciiVectors(std::istream& input, std::size_t pointCount, const std::string& name,
                      VtkBuffers& buffers)
{
    std::string type;
    input >> type;
    const bool velocity = toLower(name) == "velocity";
    if (velocity) {
        buffers.velocities.resize(pointCount);
    }
    for (std::size_t index = 0; index < pointCount; ++index) {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        if (!(input >> x >> y >> z)) {
            return false;
        }
        if (velocity) {
            buffers.velocities[index] = Vector3(x, y, z);
        }
    }
    return true;
}

bool parseAsciiPayload(std::istream& input, std::vector<Particle>& outParticles)
{
    VtkBuffers buffers;
    std::size_t pointCount = 0u;
    std::string token;
    while (input >> token) {
        if (token == "POINTS") {
            std::string type;
            input >> pointCount >> type;
            if (!isValidImportedParticleCount(pointCount)) {
                return false;
            }
            buffers.positions.resize(pointCount);
            for (Vector3& position : buffers.positions) {
                if (!(input >> position.x >> position.y >> position.z)) {
                    return false;
                }
            }
        }
        else if (token == "SCALARS" && pointCount > 0u) {
            std::string name;
            std::string type;
            int components = 1;
            input >> name >> type;
            if (!(input >> components)) {
                input.clear();
                components = 1;
            }
            if (!readAsciiScalars(input, pointCount, name, components, buffers)) {
                return false;
            }
        }
        else if (token == "VECTORS" && pointCount > 0u) {
            std::string name;
            input >> name;
            if (!readAsciiVectors(input, pointCount, name, buffers)) {
                return false;
            }
        }
    }
    return finalizeParticles(buffers, outParticles);
}

bool readBinaryScalars(std::istream& input, std::size_t pointCount, const std::string& name,
                       int components, VtkBuffers& buffers)
{
    std::string lookupLine;
    if (!std::getline(input, lookupLine)) {
        return false;
    }
    const std::size_t scalarCount =
        pointCount * static_cast<std::size_t>(std::max(1, components));
    const std::string normalizedName = toLower(name);
    if ((normalizedName == "mass" || normalizedName == "temperature") && components == 1) {
        std::vector<float>& target = normalizedName == "mass" ? buffers.masses : buffers.temperatures;
        target.resize(pointCount);
        for (float& value : target) {
            if (!readBeF32(input, value)) {
                return false;
            }
        }
    }
    else {
        float discarded = 0.0f;
        for (std::size_t index = 0; index < scalarCount; ++index) {
            if (!readBeF32(input, discarded)) {
                return false;
            }
        }
    }
    consumeOptionalLineBreak(input);
    return true;
}

bool readBinaryVectors(std::istream& input, std::size_t pointCount, const std::string& name,
                       VtkBuffers& buffers)
{
    const bool velocity = toLower(name) == "velocity";
    if (velocity) {
        buffers.velocities.resize(pointCount);
    }
    for (std::size_t index = 0; index < pointCount; ++index) {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        if (!readBeF32(input, x) || !readBeF32(input, y) || !readBeF32(input, z)) {
            return false;
        }
        if (velocity) {
            buffers.velocities[index] = Vector3(x, y, z);
        }
    }
    consumeOptionalLineBreak(input);
    return true;
}

bool parseBinaryPayload(std::istream& input, std::vector<Particle>& outParticles)
{
    VtkBuffers buffers;
    std::size_t pointCount = 0u;
    std::string line;
    while (std::getline(input, line)) {
        const std::string stripped = trim(line);
        if (stripped.empty()) {
            continue;
        }
        std::istringstream lineStream(stripped);
        std::string keyword;
        lineStream >> keyword;
        if (keyword == "POINTS") {
            std::string type;
            lineStream >> pointCount >> type;
            if (!isValidImportedParticleCount(pointCount)) {
                return false;
            }
            buffers.positions.resize(pointCount);
            for (Vector3& position : buffers.positions) {
                if (!readBeF32(input, position.x) || !readBeF32(input, position.y) ||
                    !readBeF32(input, position.z)) {
                    return false;
                }
            }
            consumeOptionalLineBreak(input);
        }
        else if (keyword == "VERTICES") {
            std::size_t cellCount = 0u;
            std::size_t totalInts = 0u;
            lineStream >> cellCount >> totalInts;
            (void)cellCount;
            std::int32_t discarded = 0;
            for (std::size_t index = 0; index < totalInts; ++index) {
                if (!readBeI32(input, discarded)) {
                    return false;
                }
            }
            consumeOptionalLineBreak(input);
        }
        else if (keyword == "SCALARS" && pointCount > 0u) {
            std::string name;
            std::string type;
            int components = 1;
            lineStream >> name >> type;
            if (!(lineStream >> components)) {
                components = 1;
            }
            if (!readBinaryScalars(input, pointCount, name, components, buffers)) {
                return false;
            }
        }
        else if (keyword == "VECTORS" && pointCount > 0u) {
            std::string name;
            lineStream >> name;
            if (!readBinaryVectors(input, pointCount, name, buffers)) {
                return false;
            }
        }
    }
    return finalizeParticles(buffers, outParticles);
}
} // namespace blitzar_simulation_vtk

bool parseVtkSnapshot(const std::string& inputPath, std::vector<Particle>& outParticles)
{
    std::ifstream input(inputPath, std::ios::binary);
    if (!input.is_open()) {
        return false;
    }
    std::string version;
    std::string title;
    std::string encoding;
    std::string dataset;
    if (!std::getline(input, version) || !std::getline(input, title) ||
        !std::getline(input, encoding) || !std::getline(input, dataset)) {
        return false;
    }
    return toLower(trim(encoding)) == "binary"
               ? blitzar_simulation_vtk::parseBinaryPayload(input, outParticles)
               : blitzar_simulation_vtk::parseAsciiPayload(input, outParticles);
}
