#include <iostream>

#ifdef _WIN32
int runVulkanApp();
#endif

int main() {
#ifdef _WIN32
    try {
        return runVulkanApp();
    } catch (const std::exception& error) {
        std::cerr << "Nie udało się uruchomić NeoCraft: " << error.what() << '\n';
        return 1;
    }
#else
    std::cout << "NeoCraft 1.0 — renderer Vulkan etapu 2 jest przeznaczony dla Windows.\n";
    return 0;
#endif
}
