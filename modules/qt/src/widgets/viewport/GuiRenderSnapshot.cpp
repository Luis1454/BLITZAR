/*
 * @file modules/qt/src/widgets/viewport/GuiRenderSnapshot.cpp
 * @brief Prepare simulation snapshots for viewport rendering.
 */

#include "src/widgets/viewport/GuiRenderSnapshot.hpp"
#include <algorithm>
#include <limits>

namespace bltzr_qt {
static constexpr int kGridSide = 16;
static constexpr int kBinCount = kGridSide * kGridSide * kGridSide;

static void centerSnapshot(std::vector<RenderParticle>& snapshot)
{
    if (snapshot.empty()) {
        return;
    }

    float minX = snapshot.front().x;
    float minY = snapshot.front().y;
    float minZ = snapshot.front().z;
    float maxX = minX;
    float maxY = minY;
    float maxZ = minZ;
    for (const RenderParticle& particle : snapshot) {
        minX = std::min(minX, particle.x);
        minY = std::min(minY, particle.y);
        minZ = std::min(minZ, particle.z);
        maxX = std::max(maxX, particle.x);
        maxY = std::max(maxY, particle.y);
        maxZ = std::max(maxZ, particle.z);
    }

    const float centerX = 0.5f * (minX + maxX);
    const float centerY = 0.5f * (minY + maxY);
    const float centerZ = 0.5f * (minZ + maxZ);
    for (RenderParticle& particle : snapshot) {
        particle.x -= centerX;
        particle.y -= centerY;
        particle.z -= centerZ;
    }
}

static std::vector<RenderParticle> spatialSample(const std::vector<RenderParticle>& input,
                                                 std::size_t cap)
{
    if (input.size() <= cap) {
        return input;
    }

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    float maxZ = std::numeric_limits<float>::lowest();
    for (const RenderParticle& particle : input) {
        minX = std::min(minX, particle.x);
        minY = std::min(minY, particle.y);
        minZ = std::min(minZ, particle.z);
        maxX = std::max(maxX, particle.x);
        maxY = std::max(maxY, particle.y);
        maxZ = std::max(maxZ, particle.z);
    }

    const float scaleX = static_cast<float>(kGridSide) / std::max(1.0e-6f, maxX - minX);
    const float scaleY = static_cast<float>(kGridSide) / std::max(1.0e-6f, maxY - minY);
    const float scaleZ = static_cast<float>(kGridSide) / std::max(1.0e-6f, maxZ - minZ);
    std::vector<int> bins(kBinCount, -1);
    for (std::size_t index = 0u; index < input.size(); ++index) {
        const RenderParticle& particle = input[index];
        const int x = std::clamp(static_cast<int>((particle.x - minX) * scaleX), 0, kGridSide - 1);
        const int y = std::clamp(static_cast<int>((particle.y - minY) * scaleY), 0, kGridSide - 1);
        const int z = std::clamp(static_cast<int>((particle.z - minZ) * scaleZ), 0, kGridSide - 1);
        const int bin = x + kGridSide * (y + kGridSide * z);
        const int selected = bins[static_cast<std::size_t>(bin)];
        if (selected < 0 || particle.mass > input[static_cast<std::size_t>(selected)].mass) {
            bins[static_cast<std::size_t>(bin)] = static_cast<int>(index);
        }
    }

    std::vector<RenderParticle> result;
    result.reserve(cap);
    std::vector<unsigned char> selected(input.size(), 0u);
    for (const int index : bins) {
        if (index < 0 || result.size() >= cap) {
            continue;
        }
        result.push_back(input[static_cast<std::size_t>(index)]);
        selected[static_cast<std::size_t>(index)] = 1u;
    }

    const std::size_t stride = std::max<std::size_t>(1u, (input.size() + cap - 1u) / cap);
    for (std::size_t index = 0u; index < input.size() && result.size() < cap; index += stride) {
        if (selected[index] == 0u) {
            result.push_back(input[index]);
        }
    }
    return result;
}

std::vector<RenderParticle> prepareRenderSnapshot(std::vector<RenderParticle> snapshot,
                                                  std::size_t cap)
{
    centerSnapshot(snapshot);
    return spatialSample(snapshot, std::max<std::size_t>(2u, cap));
}
} // namespace bltzr_qt
