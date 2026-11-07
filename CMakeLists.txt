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

# Versione grafica (Qt 6, oppure Qt 5.15). Se Qt non è installato viene semplicemente saltata.
option(CATALOGO_GUI "Compila anche la versione grafica (richiede Qt)" ON)
if(CATALOGO_GUI)
  find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets QUIET)
  if(QT_FOUND)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
    add_executable(catalogo_gui WIN32 MACOSX_BUNDLE
      gui/main.cpp
      gui/mainwindow.cpp gui/mainwindow.h
      gui/componentmodel.cpp gui/componentmodel.h
      gui/componentdialog.cpp gui/componentdialog.h
      gui/colorcodedialog.cpp gui/colorcodedialog.h
      gui/statisticsdialog.cpp gui/statisticsdialog.h
      gui/resistorwidget.cpp gui/resistorwidget.h
      gui/theme.cpp gui/theme.h
      gui/qtutil.h
    )
    set_target_properties(catalogo_gui PROPERTIES AUTOMOC ON)
    target_link_libraries(catalogo_gui PRIVATE catalogo_core Qt${QT_VERSION_MAJOR}::Widgets)
    message(STATUS "Versione grafica: Qt ${QT_VERSION}")
  else()
    message(STATUS "Qt non trovato: la versione grafica (catalogo_gui) non verrà compilata")
  endif()
endif()

# Test automatici: cmake --build build && ctest --test-dir build
enable_testing()
add_executable(test_catalogo tests/test_main.cpp)
target_link_libraries(test_catalogo PRIVATE catalogo_core)
add_test(NAME test_catalogo COMMAND test_catalogo)
