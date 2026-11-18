#include "neocraft/world/chunk_coord.hpp"

#include <functional>

namespace neocraft::world {

std::size_t ChunkCoordHash::operator()(const ChunkCoord& coord) const noexcept {
    std::size_t seed = std::hash<std::int32_t>{}(coord.x);
    const auto combine = [&seed](std::int32_t value) {
        const std::size_t hashed = std::hash<std::int32_t>{}(value);
        seed ^= hashed + static_cast<std::size_t>(0x9e3779b9U) + (seed << 6U) + (seed >> 2U);
    };
    combine(coord.y);
    combine(coord.z);
    return seed;
}

} // namespace neocraft::world
