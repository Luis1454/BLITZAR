/*
 * @file engine/src/server/simulation_server/ParseVtk.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Source artifact for the BLITZAR simulation project.
 */

#include "Internal.hpp"

/*
 * @brief Documents the parse vtk snapshot operation contract.
 * @param inputPath Input value used by this contract.
 * @param outParticles Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool parseVtkSnapshot(const std::string& inputPath, std::vector<Particle>& outParticles)
{
    auto validatePointCount = [](std::size_t pointCount) {
        return isValidImportedParticleCount(pointCount);
    };
    auto finalizeParticles = [&outParticles](const std::vector<Vector3>& positions,
                                             const std::vector<float>& masses,
                                             const std::vector<float>& temperatures,
                                             const std::vector<Vector3>& velocities) {
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
    auto parseAsciiPayload = [&](std::istream& in) {
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
                const std::size_t scalarCount =
                    pointCount * static_cast<std::size_t>(std::max(1, components));
                if (toLower(name) == "mass" && components == 1) {
                    masses.resize(pointCount);
                    for (std::size_t i = 0; i < pointCount; ++i) {
                        if (!(in >> masses[i])) {
                            return false;
                        }
                    }
                }
                else if (toLower(name) == "temperature" && components == 1) {
                    temperatures.resize(pointCount);
                    for (std::size_t i = 0; i < pointCount; ++i) {
                        if (!(in >> temperatures[i])) {
                            return false;
                        }
                    }
                }
                else {
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
                }
                else {
                    if (!skipVectors(pointCount)) {
                        return false;
                    }
                }
                continue;
            }
        }
        return finalizeParticles(positions, masses, temperatures, velocities);
    };
    auto parseBinaryPayload = [&](std::istream& in) {
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
            if (keyword == "POINT_DATA")
                continue;
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
                const std::size_t scalarCount =
                    pointCount * static_cast<std::size_t>(std::max(1, components));
                if (toLower(name) == "mass" && components == 1) {
                    masses.resize(pointCount);
                    for (std::size_t i = 0; i < pointCount; ++i) {
                        if (!readBeF32(in, masses[i])) {
                            return false;
                        }
                    }
                }
                else if (toLower(name) == "temperature" && components == 1) {
                    temperatures.resize(pointCount);
                    for (std::size_t i = 0; i < pointCount; ++i) {
                        if (!readBeF32(in, temperatures[i])) {
                            return false;
                        }
                    }
                }
                else {
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
                }
                else {
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
    if (!std::getline(in, line1) || !std::getline(in, line2) || !std::getline(in, line3) ||
        !std::getline(in, line4)) {
        return false;
    }
    const std::string encoding = toLower(trim(line3));
    if (encoding == "binary") {
        return parseBinaryPayload(in);
    }
    return parseAsciiPayload(in);
}
