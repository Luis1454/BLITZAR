#include "bvh/BvhIndex.hpp"

#include "neighborhood/NeighborDistance.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <numeric>

namespace {

constexpr std::uint64_t kHashOffset = 1469598103934665603ULL;
constexpr std::uint64_t kHashPrime = 1099511628211ULL;

void Mix(std::uint64_t& hash, std::uint64_t value) noexcept
{
    hash ^= value;
    hash *= kHashPrime;
}

bool IsFinite(blitzar_core::Vector3 value) noexcept
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

void MixVector(std::uint64_t& hash, blitzar_core::Vector3 value) noexcept
{
    Mix(hash, std::bit_cast<std::uint64_t>(value.x));
    Mix(hash, std::bit_cast<std::uint64_t>(value.y));
    Mix(hash, std::bit_cast<std::uint64_t>(value.z));
}

} // namespace

namespace blitzar_bvh {

bool BvhNode::IsLeaf() const noexcept
{
    return left == InvalidIndex && right == InvalidIndex;
}

std::size_t BvhWorkspace::MemoryBytes() const noexcept
{
    return stack.capacity() * sizeof(std::size_t) + candidates.capacity() * sizeof(std::size_t);
}

BvhIndex::BvhIndex(std::size_t leaf_size) : leaf_size_(leaf_size) {}

bool BvhIndex::IsValidFrame(const blitzar_neighborhood::NeighborFrame& frame) noexcept
{
    if (frame.x.empty() || frame.x.size() != frame.y.size() || frame.x.size() != frame.z.size()) {
        return false;
    }

    for (std::size_t index = 0; index < frame.x.size(); ++index) {
        if (!IsFinite(frame.Position(index))) {
            return false;
        }
    }

    return true;
}

BvhNode BvhIndex::ComputeBounds(const blitzar_neighborhood::NeighborFrame& frame, std::size_t begin,
    std::size_t end) const noexcept
{
    BvhNode result{};
    const blitzar_core::Vector3 first = frame.Position(indices_[begin]);

    result.minimum = first;
    result.maximum = first;

    for (std::size_t cursor = begin + 1U; cursor < end; ++cursor) {
        const blitzar_core::Vector3 position = frame.Position(indices_[cursor]);

        result.minimum.x = std::min(result.minimum.x, position.x);
        result.minimum.y = std::min(result.minimum.y, position.y);
        result.minimum.z = std::min(result.minimum.z, position.z);
        result.maximum.x = std::max(result.maximum.x, position.x);
        result.maximum.y = std::max(result.maximum.y, position.y);
        result.maximum.z = std::max(result.maximum.z, position.z);
    }

    return result;
}

blitzar_core::Scalar BvhIndex::Coordinate(blitzar_core::Vector3 position, int axis) noexcept
{
    if (axis == 0) {
        return position.x;
    }
    if (axis == 1) {
        return position.y;
    }

    return position.z;
}

int BvhIndex::LongestAxis(const BvhNode& node) noexcept
{
    const blitzar_core::Scalar x = node.maximum.x - node.minimum.x;
    const blitzar_core::Scalar y = node.maximum.y - node.minimum.y;
    const blitzar_core::Scalar z = node.maximum.z - node.minimum.z;

    if (x >= y && x >= z) {
        return 0;
    }
    if (y >= z) {
        return 1;
    }

    return 2;
}

blitzar_core::Scalar BvhIndex::DistanceToAxis(
    blitzar_core::Scalar value, blitzar_core::Scalar minimum, blitzar_core::Scalar maximum) noexcept
{
    if (value < minimum) {
        return minimum - value;
    }
    if (value > maximum) {
        return value - maximum;
    }

    return 0.0;
}

bool BvhIndex::Intersects(
    const BvhNode& node, blitzar_core::Vector3 point, blitzar_core::Scalar radius_squared) noexcept
{
    const blitzar_core::Scalar x = DistanceToAxis(point.x, node.minimum.x, node.maximum.x);
    const blitzar_core::Scalar y = DistanceToAxis(point.y, node.minimum.y, node.maximum.y);
    const blitzar_core::Scalar z = DistanceToAxis(point.z, node.minimum.z, node.maximum.z);

    return x * x + y * y + z * z <= radius_squared;
}

bool BvhIndex::Build(const blitzar_neighborhood::NeighborFrame& frame)
{
    if (!IsValidFrame(frame) || leaf_size_ == 0 ||
        frame.x.size() > std::numeric_limits<std::size_t>::max() / 2U + 1U) {
        built_ = false;

        return false;
    }

    particle_count_ = frame.x.size();
    built_ = false;

    nodes_.clear();
    indices_.resize(particle_count_);
    tasks_.clear();
    nodes_.reserve(particle_count_ * 2U - 1U);
    tasks_.reserve(particle_count_);
    std::iota(indices_.begin(), indices_.end(), 0U);
    nodes_.push_back({});
    tasks_.push_back({0, particle_count_, 0});

    while (!tasks_.empty()) {
        const BuildTask task = tasks_.back();

        tasks_.pop_back();

        const BvhNode bounds = ComputeBounds(frame, task.begin, task.end);

        nodes_[task.node].minimum = bounds.minimum;
        nodes_[task.node].maximum = bounds.maximum;

        if (task.end - task.begin <= leaf_size_) {
            nodes_[task.node].begin = task.begin;
            nodes_[task.node].count = task.end - task.begin;

            continue;
        }

        const int axis = LongestAxis(bounds);
        const std::size_t middle = task.begin + (task.end - task.begin) / 2U;

        std::stable_sort(indices_.begin() + static_cast<std::ptrdiff_t>(task.begin),
            indices_.begin() + static_cast<std::ptrdiff_t>(task.end),
            [&frame, axis](std::size_t left, std::size_t right) noexcept {
                const blitzar_core::Scalar left_coordinate = Coordinate(frame.Position(left), axis);
                const blitzar_core::Scalar right_coordinate =
                    Coordinate(frame.Position(right), axis);

                return left_coordinate < right_coordinate ||
                       (left_coordinate == right_coordinate && left < right);
            });

        const std::size_t left = nodes_.size();

        nodes_.push_back({});

        const std::size_t right = nodes_.size();

        nodes_.push_back({});

        nodes_[task.node].left = left;
        nodes_[task.node].right = right;

        tasks_.push_back({middle, task.end, right});
        tasks_.push_back({task.begin, middle, left});
    }

    built_ = !nodes_.empty();

    return built_;
}

bool BvhIndex::Refit(const blitzar_neighborhood::NeighborFrame& frame)
{
    if (!built_ || frame.x.size() != particle_count_ || !IsValidFrame(frame)) {
        return false;
    }

    for (std::size_t index = 0; index < nodes_.size(); ++index) {
        BvhNode& node = nodes_[index];

        if (node.IsLeaf()) {
            const BvhNode bounds = ComputeBounds(frame, node.begin, node.begin + node.count);

            node.minimum = bounds.minimum;
            node.maximum = bounds.maximum;
        }
    }

    for (std::size_t index = nodes_.size(); index-- > 0;) {
        BvhNode& node = nodes_[index];

        if (node.IsLeaf() || node.left >= nodes_.size() || node.right >= nodes_.size()) {
            continue;
        }

        const BvhNode& left = nodes_[node.left];
        const BvhNode& right = nodes_[node.right];
        node.minimum = {std::min(left.minimum.x, right.minimum.x),
            std::min(left.minimum.y, right.minimum.y), std::min(left.minimum.z, right.minimum.z)};

        node.maximum = {std::max(left.maximum.x, right.maximum.x),
            std::max(left.maximum.y, right.maximum.y), std::max(left.maximum.z, right.maximum.z)};
    }

    return true;
}

blitzar_neighborhood::NeighborSet BvhIndex::Query(const blitzar_neighborhood::NeighborFrame& frame,
    blitzar_core::Scalar radius, BvhWorkspace& workspace) const
{
    blitzar_neighborhood::NeighborSet result;

    if (!built_ || frame.x.size() != particle_count_ || !IsValidFrame(frame) ||
        !std::isfinite(radius) || radius <= 0.0) {
        return result;
    }

    workspace.stack.reserve(nodes_.size());
    workspace.candidates.reserve(32U);
    result.offsets.resize(particle_count_ + 1U);
    result.indices.reserve(particle_count_);

    const blitzar_core::Scalar radius_squared = radius * radius;

    for (std::size_t target = 0; target < particle_count_; ++target) {
        workspace.stack.clear();
        workspace.candidates.clear();
        workspace.stack.push_back(0U);

        const blitzar_core::Vector3 point = frame.Position(target);

        while (!workspace.stack.empty()) {
            const std::size_t node_index = workspace.stack.back();

            workspace.stack.pop_back();

            const BvhNode& node = nodes_[node_index];

            if (!Intersects(node, point, radius_squared)) {
                continue;
            }
            if (node.IsLeaf()) {
                for (std::size_t offset = 0; offset < node.count; ++offset) {
                    const std::size_t source = indices_[node.begin + offset];

                    if (source != target && blitzar_neighborhood::SquaredDistance(
                                                point, frame.Position(source)) <= radius_squared) {
                        workspace.candidates.push_back(source);
                    }
                }

                continue;
            }

            if (node.right < nodes_.size()) {
                workspace.stack.push_back(node.right);
            }
            if (node.left < nodes_.size()) {
                workspace.stack.push_back(node.left);
            }
        }

        std::sort(workspace.candidates.begin(), workspace.candidates.end());
        result.indices.insert(
            result.indices.end(), workspace.candidates.begin(), workspace.candidates.end());

        result.offsets[target + 1U] = result.indices.size();
    }

    return result;
}

std::size_t BvhIndex::MemoryBytes() const noexcept
{
    return nodes_.capacity() * sizeof(BvhNode) + indices_.capacity() * sizeof(std::size_t) +
           tasks_.capacity() * sizeof(BuildTask);
}

std::size_t BvhIndex::ParticleCount() const noexcept
{
    return particle_count_;
}

std::size_t BvhIndex::LeafSize() const noexcept
{
    return leaf_size_;
}

std::uint64_t BvhIndex::Hash() const noexcept
{
    std::uint64_t hash = kHashOffset;

    Mix(hash, static_cast<std::uint64_t>(leaf_size_));
    Mix(hash, static_cast<std::uint64_t>(particle_count_));
    Mix(hash, built_ ? 1U : 0U);

    for (const BvhNode& node : nodes_) {
        MixVector(hash, node.minimum);
        MixVector(hash, node.maximum);
        Mix(hash, static_cast<std::uint64_t>(node.left));
        Mix(hash, static_cast<std::uint64_t>(node.right));
        Mix(hash, static_cast<std::uint64_t>(node.begin));
        Mix(hash, static_cast<std::uint64_t>(node.count));
    }
    for (const std::size_t index : indices_) {
        Mix(hash, static_cast<std::uint64_t>(index));
    }

    return hash;
}

} // namespace blitzar_bvh
