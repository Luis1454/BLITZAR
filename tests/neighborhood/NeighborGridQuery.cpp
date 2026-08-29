#include "neighborhood/NeighborDistance.hpp"
#include "neighborhood/NeighborGrid.hpp"

#include <algorithm>
#include <vector>

namespace {

bool SameCell(blitzar_neighborhood::GridCell left, blitzar_neighborhood::GridCell right) noexcept
{
    return left.x == right.x && left.y == right.y && left.z == right.z;
}

} // namespace

namespace blitzar_neighborhood {

NeighborSet GridIndex::Query(const NeighborFrame& frame) const
{
    NeighborSet result;

    result.offsets.resize(frame.x.size() + 1U);

    std::vector<std::size_t> candidates;

    candidates.reserve(32U);

    const double radius_squared = parameters_.radius * parameters_.radius;

    for (std::size_t target = 0; target < frame.x.size(); ++target) {
        candidates.clear();

        const GridCell center = Locate(frame.Position(target));

        for (int x = -1; x <= 1; ++x) {
            for (int y = -1; y <= 1; ++y) {
                for (int z = -1; z <= 1; ++z) {
                    const GridCell cell{center.x + x, center.y + y, center.z + z};

                    if (cell.x < 0 || cell.y < 0 || cell.z < 0 || cell.x >= dimensions_.x ||
                        cell.y >= dimensions_.y || cell.z >= dimensions_.z) {
                        continue;
                    }

                    const std::size_t range_index = RangeIndex(cell);

                    if (range_index == ranges_.size()) {
                        continue;
                    }

                    const GridRange& range = ranges_[range_index];

                    for (std::size_t entry = range.begin; entry < range.end; ++entry) {
                        const std::size_t source = entries_[entry].particle;

                        if (source != target && SquaredDistance(frame.Position(target),
                                                    frame.Position(source)) <= radius_squared) {
                            candidates.push_back(source);
                        }
                    }
                }
            }
        }

        std::sort(candidates.begin(), candidates.end());
        result.indices.insert(result.indices.end(), candidates.begin(), candidates.end());

        result.offsets[target + 1U] = result.indices.size();
    }

    return result;
}

std::size_t GridIndex::RangeIndex(GridCell cell) const noexcept
{
    const std::uint64_t key = Key(cell);
    const auto iterator = std::lower_bound(ranges_.begin(), ranges_.end(), key,
        [](const GridRange& range, std::uint64_t value) noexcept { return range.key < value; });

    if (iterator == ranges_.end() || iterator->key != key) {
        return ranges_.size();
    }

    const std::size_t index = static_cast<std::size_t>(iterator - ranges_.begin());

    if (SameCell(iterator->cell, cell)) {
        return index;
    }

    for (std::size_t cursor = index + 1U; cursor < ranges_.size() && ranges_[cursor].key == key;
        ++cursor) {
        if (SameCell(ranges_[cursor].cell, cell)) {
            return cursor;
        }
    }

    return ranges_.size();
}

void GridIndex::BuildRanges()
{
    ranges_.clear();

    if (entries_.empty()) {
        return;
    }

    ranges_.reserve(entries_.size());

    std::size_t begin = 0;

    while (begin < entries_.size()) {
        std::size_t end = begin + 1U;

        while (end < entries_.size() && entries_[end].key == entries_[begin].key &&
               SameCell(entries_[end].cell, entries_[begin].cell)) {
            ++end;
        }

        ranges_.push_back({entries_[begin].cell, entries_[begin].key, begin, end});

        begin = end;
    }
}

} // namespace blitzar_neighborhood
