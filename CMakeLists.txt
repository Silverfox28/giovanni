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
  if(WIN32)
    # Su Windows Qt si trova di solito in C:\Qt\<versione>\<kit>: lo si cerca lì automaticamente,
    # scegliendo il kit adatto al compilatore (msvc... per Visual C++, mingw... per MinGW).
    # Si può indicare un'altra cartella con la variabile d'ambiente QTDIR o con -DCMAKE_PREFIX_PATH=...
    if(DEFINED ENV{QTDIR})
      list(APPEND CMAKE_PREFIX_PATH "$ENV{QTDIR}")
    endif()
    if(MSVC)
      file(GLOB _qt_kits "C:/Qt/6.*/msvc*_64")
    else()
      file(GLOB _qt_kits "C:/Qt/6.*/mingw*_64")
    endif()
    if(_qt_kits)
      if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.18)
        list(SORT _qt_kits COMPARE NATURAL ORDER DESCENDING)  # prima la versione più recente
      else()
        list(SORT _qt_kits ORDER DESCENDING)
      endif()
      list(APPEND CMAKE_PREFIX_PATH ${_qt_kits})
    endif()
  endif()

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

    if(WIN32)
      # Dopo ogni compilazione copia accanto a catalogo_gui.exe le librerie (DLL) di Qt,
      # così il programma si avvia direttamente da Visual Studio o con un doppio clic.
      set(_qt_bin_hints "${Qt${QT_VERSION_MAJOR}_DIR}/../../../bin")
      if(TARGET Qt${QT_VERSION_MAJOR}::qmake)
        get_target_property(_qmake Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
        if(NOT _qmake)
          get_target_property(_qmake Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION_RELEASE)
        endif()
        if(_qmake)
          get_filename_component(_qmake_dir "${_qmake}" DIRECTORY)
          list(APPEND _qt_bin_hints "${_qmake_dir}")
        endif()
      endif()
      find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${_qt_bin_hints} NO_DEFAULT_PATH)
      if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET catalogo_gui POST_BUILD
          COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --no-system-d3d-compiler --no-opengl-sw
                  "$<TARGET_FILE:catalogo_gui>"
          COMMENT "Copia delle librerie di Qt accanto a catalogo_gui.exe"
          VERBATIM)
      else()
        message(WARNING "windeployqt non trovato: per avviare catalogo_gui.exe servirà copiare a mano le DLL di Qt")
      endif()
    endif()
  else()
    message(STATUS "Qt non trovato: la versione grafica (catalogo_gui) non verrà compilata")
  endif()
endif()

# Test automatici: cmake --build build && ctest --test-dir build
enable_testing()
add_executable(test_catalogo tests/test_main.cpp)
target_link_libraries(test_catalogo PRIVATE catalogo_core)
add_test(NAME test_catalogo COMMAND test_catalogo)
