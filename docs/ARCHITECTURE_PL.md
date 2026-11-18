# Architektura NeoCraft — stan po etapie 1

## Zasada integracji

Aplikacja C++ jest hostem i właścicielem pętli gry. Moduły pomocnicze nie będą wywoływane bezpośrednio w każdej klatce, jeśli ich zadanie można wykonać jako osobne narzędzie lub etap przygotowania danych. Każda integracja otrzyma określony format danych, obsługę błędów i testy.

## Plan granic modułów

| Język | Odpowiedzialność | Planowana granica integracji |
|---|---|---|
| C++20 | Aplikacja, renderer Vulkan, wejście i logika | Główna aplikacja i biblioteki silnika |
| Rust | Dane bloków/chunków i serializacja świata | Biblioteka C ABI lub jawny format pliku; wybór po prototypie |
| Julia | Generowanie terenu, biomów i struktur | Narzędzie offline eksportujące wersjonowany format świata |
| Fortran | Wybrane, zmierzone obliczenia numeryczne | Biblioteka z wąskim interfejsem C ABI, jeśli benchmark uzasadni integrację |
| Ada | Walidacja konfiguracji i integralności danych | Narzędzie walidujące pliki przed ich użyciem |
| Scilab | Eksperymenty i analiza parametrów algorytmów | Skrypty badawcze, bez zależności w binarnym wydaniu gry |

Powyższe granice są planem, nie deklaracją ukończonych integracji. Nie zostaną dodane puste funkcje ani nieużywane zależności tylko po to, aby wymienić dany język.

## Etap 1

CMake buduje `neocraft_core`, aplikację `neocraft` oraz opcjonalny test CTest. Model zawiera identyfikatory kilku przykładowych bloków i haszowalne współrzędne chunków; nie jest to jeszcze implementacja świata voxelowego.
