# NeoCraft 1.0 — etap 1

NeoCraft to niezależny projekt gry voxelowej 3D dla Windows 10/11, rozwijany etapami. Ten pierwszy etap dostarcza czysty, kompilowalny szkielet C++20 z CMake/Ninja i testem podstawowego modelu współrzędnych chunków. Nie zawiera jeszcze renderera Vulkan ani gotowej gry.

## Wymagania etapu 1

- CMake 3.24 lub nowszy
- Ninja
- Kompilator C++20 (Visual Studio 2022/MSVC, Clang lub GCC)
- Git (opcjonalnie)

Sprawdź, co jest dostępne w PowerShell:

```powershell
cmake --version
 ninja --version
 c++ --version
```

W przypadku MSVC sprawdź kompilator poleceniem `cl` w **Developer PowerShell for VS 2022**. Nie instaluj ponownie narzędzi, które już działają.

## Konfiguracja, kompilacja i testy

Uruchom w katalogu głównym repozytorium:

```powershell
cmake --preset windows-ninja
cmake --build --preset windows-ninja
ctest --preset windows-ninja
```

Preset `windows-ninja` używa generatora Ninja i buduje konfigurację Debug. Jeśli pracujesz na innym systemie, użyj odpowiednio presetu `default-ninja`.

Uruchomienie programu:

```powershell
.\build\windows-ninja\neocraft.exe
```

Oczekiwany komunikat: `NeoCraft 1.0 — szkielet silnika C++20`.

## Struktura

- `app/` — punkt wejścia aplikacji.
- `engine/world/` — podstawowe typy współrzędnych świata.
- `modules/` — ustalone granice odpowiedzialności planowanych modułów wielojęzykowych; każdy będzie dodawany, gdy będzie miał działającą funkcję.
- `tests/` — testy CTest.
- `assets/`, `data/`, `docs/`, `scripts/` — zasoby, dane, dokumentacja i narzędzia projektu.

## Podział modułów

C++ będzie hostem aplikacji i renderera Vulkan. Rust będzie biblioteką danych świata, Julia narzędziem generującym światy, Fortran modułem obliczeń numerycznych, Ada walidatorem danych, a Scilab środowiskiem eksperymentów algorytmicznych. Integracje zostaną wprowadzane dopiero wraz z konkretnymi interfejsami i testami — nie są jeszcze częścią etapu 1.

Szczegóły architektury oraz plan etapów znajdują się w [`docs/ARCHITECTURE_PL.md`](docs/ARCHITECTURE_PL.md).
