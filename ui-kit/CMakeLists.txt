cmake_minimum_required(VERSION 3.16)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets)

set(UI_KIT_THEME_SOURCES
    theme/tokens.h
    theme/theme.h
    theme/theme.cpp
    theme/style_helper.h
    theme/animation_helper.h
)

set(UI_KIT_COMPONENT_SOURCES
    components/button.h
    components/button.cpp
    components/badge.h
    components/badge.cpp
    components/spinner.h
    components/spinner.cpp
    components/card.h
    components/card.cpp
    components/input.h
    components/input.cpp
    components/switch.h
    components/switch.cpp
    components/slider.h
    components/slider.cpp
    components/select.h
    components/select.cpp
    components/form_field.h
    components/form_field.cpp
    components/separator.h
    components/separator.cpp
    components/tabs.h
    components/tabs.cpp
    components/accordion.h
    components/accordion.cpp
    components/sidebar.h
    components/sidebar.cpp
)

add_library(ui_kit STATIC
    ${UI_KIT_THEME_SOURCES}
    ${UI_KIT_COMPONENT_SOURCES}
)

target_include_directories(ui_kit PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ui_kit PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)

