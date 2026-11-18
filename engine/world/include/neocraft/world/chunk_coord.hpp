#pragma once

#include <cstddef>
#include <cstdint>

namespace neocraft::world {

struct ChunkCoord {
    std::int32_t x{};
    std::int32_t y{};
    std::int32_t z{};

    friend constexpr bool operator==(const ChunkCoord&, const ChunkCoord&) = default;
};

struct ChunkCoordHash {
    [[nodiscard]] std::size_t operator()(const ChunkCoord& coord) const noexcept;
};

enum class BlockId : std::uint16_t {
    Air = 0,
    Grass = 1,
    Dirt = 2,
    Stone = 3,
    Water = 4,
};

} // namespace neocraft::world
