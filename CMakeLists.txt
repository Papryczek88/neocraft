cmake_minimum_required(VERSION 3.24)

project(NeoCraft VERSION 1.0.0 LANGUAGES CXX)

option(NEOCRAFT_BUILD_TESTS "Build NeoCraft tests" ON)

add_library(neocraft_core
    engine/world/src/chunk_coord.cpp
)
target_include_directories(neocraft_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/world/include>
)
target_compile_features(neocraft_core PUBLIC cxx_std_20)

add_executable(neocraft app/src/main.cpp)
target_link_libraries(neocraft PRIVATE neocraft_core)

if(MSVC)
    target_compile_options(neocraft_core PRIVATE /W4 /permissive-)
    target_compile_options(neocraft PRIVATE /W4 /permissive-)
else()
    target_compile_options(neocraft_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(neocraft PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(NEOCRAFT_BUILD_TESTS)
    enable_testing()
    add_executable(neocraft_core_tests tests/core_tests.cpp)
    target_link_libraries(neocraft_core_tests PRIVATE neocraft_core)
    if(MSVC)
        target_compile_options(neocraft_core_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(neocraft_core_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME neocraft_core_tests COMMAND neocraft_core_tests)
endif()
