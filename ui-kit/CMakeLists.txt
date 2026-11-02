cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(ui_kit VERSION 1.0.0 LANGUAGES CXX)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Svg)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Svg)

set(UI_KIT_THEME_SOURCES
    theme/tokens.h
    theme/theme.h
    theme/theme.cpp
    theme/style_helper.h
    theme/animation_helper.h
    theme/icon_helper.h
    theme/icon_helper.cpp
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
    components/toast.h
    components/toast.cpp
    components/dialog.h
    components/dialog.cpp
    components/tooltip.h
    components/tooltip.cpp
    components/progress.h
    components/progress.cpp
    components/avatar.h
    components/avatar.cpp
    components/table.h
    components/table.cpp
    components/checkbox.h
    components/checkbox.cpp
    components/radio_group.h
    components/radio_group.cpp
    components/textarea.h
    components/textarea.cpp
    components/number_input.h
    components/number_input.cpp
    components/combobox.h
    components/combobox.cpp
    components/menu.h
    components/menu.cpp
    components/alert.h
    components/alert.cpp
    components/popover.h
    components/popover.cpp
    components/scroll_area.h
    components/scroll_area.cpp
    components/splitter.h
    components/splitter.cpp
    components/empty_state.h
    components/empty_state.cpp
    demo/demo_page.h
    demo/demo_page.cpp
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
    Qt${QT_VERSION_MAJOR}::Svg
)

option(UI_KIT_BUILD_DEMO "Build standalone UI-Kit demo showcase executable" OFF)
if(UI_KIT_BUILD_DEMO)
    set(UI_KIT_DEMO_RESOURCES "")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../resources/icons.qrc")
        list(APPEND UI_KIT_DEMO_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../resources/icons.qrc")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons.qrc")
        list(APPEND UI_KIT_DEMO_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons.qrc")
    endif()
    add_executable(ui_kit_demo
        demo/main.cpp
        ${UI_KIT_DEMO_RESOURCES}
    )
    target_link_libraries(ui_kit_demo PRIVATE ui_kit)
endif()
