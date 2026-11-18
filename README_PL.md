# NeoCraft 1.0 — etap 2: okno i inicjalizacja Vulkan

NeoCraft to niezależna gra voxelowa 3D rozwijana etapami. Etap 2 dodaje na Windows prawdziwe okno Win32, inicjalizację Vulkan, wybór GPU i kolejki, swapchain, render pass oraz ciągłe prezentowanie obrazu czyszczonego przez GPU. To jeszcze nie renderuje bloków ani nie implementuje gameplayu.

## Wymagania Windows

- Windows 10/11
- CMake 3.24+, Ninja
- Visual Studio 2022 z kompilatorem C++20 (uruchom Developer PowerShell for VS 2022)
- Vulkan SDK z nagłówkami i loaderem oraz aktualny sterownik GPU obsługujący Vulkan

Sprawdź narzędzia i Vulkan SDK w PowerShell:

```powershell
cmake --version
ninja --version
cl
$env:VULKAN_SDK
Test-Path "$env:VULKAN_SDK\Include\vulkan\vulkan.h"
```

Jeśli `VULKAN_SDK` jest puste lub `Test-Path` zwraca `False`, CMake nie znajdzie SDK. Instaluj brakujące narzędzie tylko wtedy, gdy rzeczywiście go brakuje. W razie problemu wklej pełny błąd konfiguracji.

## Pobranie, budowanie i testy

W PowerShell:

```powershell
Set-Location $HOME
if (Test-Path .\neocraft\.git) {
    git -C .\neocraft pull
} else {
    git clone https://github.com/Papryczek88/neocraft.git .\neocraft
}
Set-Location .\neocraft
cmake --preset windows-ninja
cmake --build --preset windows-ninja
ctest --preset windows-ninja
.\build\windows-ninja\neocraft.exe
```

Powinno otworzyć się okno NeoCraft z jednolitym, ciemnoturkusowym tłem. To prawdziwy render pass i swapchain Vulkan, ale jeszcze bez geometrii. Zamknij okno lub naciśnij Escape.

## Zakres etapu 2

- Win32 window i Vulkan Win32 surface.
- Vulkan instance, fizyczne urządzenie z kolejką grafiki/prezentacji, logiczne urządzenie.
- Swapchain, widoki obrazów, render pass, framebuffer i command buffers.
- Synchronizacja acquire/submit/present oraz odtwarzanie swapchain po zmianie rozmiaru.
- Brak shaderów w tym etapie: ekran jest czyszczony attachmentem render passa.

Etap nie jest uznany za zweryfikowany, dopóki konfiguracja, kompilacja, test i uruchomienie nie przejdą na docelowym Windowsie. Renderer w tym etapie jest Windows-only; logika core i testy nadal budują się na innych platformach.
