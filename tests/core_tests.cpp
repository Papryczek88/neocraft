#include "neocraft/world/chunk_coord.hpp"

#include <iostream>
#include <unordered_set>

int main() {
    using neocraft::world::ChunkCoord;
    using neocraft::world::ChunkCoordHash;

    const ChunkCoord origin{0, 0, 0};
    const ChunkCoord east{1, 0, 0};
    const ChunkCoord originCopy{0, 0, 0};
    std::unordered_set<ChunkCoord, ChunkCoordHash> chunks;
    chunks.insert(origin);
    chunks.insert(east);
    chunks.insert(originCopy);

    if (!(origin == originCopy) || origin == east || chunks.size() != 2U) {
        std::cerr << "Test współrzędnych chunków nie powiódł się\n";
        return 1;
    }

    std::cout << "Test współrzędnych chunków zakończony powodzeniem\n";
    return 0;
}
