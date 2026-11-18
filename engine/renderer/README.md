# Renderer Vulkan

Kod renderera Windows znajduje się w `src/vulkan_app.cpp`. Etap 2 tworzy okno Win32, instance/surface, wybiera urządzenie i kolejkę, prezentuje swapchain przez render pass czyszczący ekran. Geometria, shadery, kamera i chunki będą dodawane w następnych etapach. Wymagany jest Vulkan SDK wykrywany przez CMake.
