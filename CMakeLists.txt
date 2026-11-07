cmake_minimum_required(VERSION 3.14)
project(CatalogoComponenti VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Logica del catalogo (senza interfaccia), condivisa da programma e test.
add_library(catalogo_core STATIC
  src/textutil.cpp
  src/units.cpp
  src/colorcode.cpp
  src/csv.cpp
  src/component.cpp
  src/inventory.cpp
)
target_include_directories(catalogo_core PUBLIC include)

if(MSVC)
  # /utf-8: i sorgenti contengono caratteri come "à", "Ω" e "€".
  target_compile_options(catalogo_core PUBLIC /W4 /utf-8)
else()
  target_compile_options(catalogo_core PUBLIC -Wall -Wextra -Wpedantic)
endif()

add_executable(catalogo
  src/main.cpp
  src/app.cpp
  src/console.cpp
)
target_link_libraries(catalogo PRIVATE catalogo_core)

# Test automatici: cmake --build build && ctest --test-dir build
enable_testing()
add_executable(test_catalogo tests/test_main.cpp)
target_link_libraries(test_catalogo PRIVATE catalogo_core)
add_test(NAME test_catalogo COMMAND test_catalogo)
