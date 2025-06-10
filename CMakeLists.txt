cmake_minimum_required(VERSION 3.16)

project(ImageConverter VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Pdf)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Pdf)

set(PROJECT_WIDGETS
        widgets/button_action.cpp
        widgets/button_action.h
        widgets/button_toggle.cpp
        widgets/button_toggle.h
        widgets/input_dropdown.cpp
        widgets/input_dropdown.h
        widgets/input_field.cpp
        widgets/input_field.h
        widgets/input_upload.cpp
        widgets/input_upload.h
        widgets/input_invalid.cpp
        widgets/input_invalid.h
        widgets/input_widget.cpp
        widgets/input_widget.h
        widgets/sidebar_panel.cpp
        widgets/sidebar_panel.h
        widgets/button_sidebar_active.cpp
        widgets/button_sidebar_active.h
        widgets/button_sidebar_widget.cpp
        widgets/button_sidebar_widget.h
        widgets/drop_file_widget.h
        widgets/drop_file_widget.cpp
        widgets/slider_widget.h
        widgets/slider_widget.cpp
        widgets/message_box_widget.cpp
        widgets/message_box_widget.h
)

set(PROJECT_STYLES
        styles/colors.h
        styles/text_style.h
        resources/input_type.h
        resources/icon_type.h
        resources/icons.qrc
)

set(
        PROJECT_PAGES
        pages/main_page.cpp
        pages/main_page.h
        pages/pdf_page.cpp
        pages/pdf_page.h
)

set(PROJECT_SOURCES
        main.cpp
        mainwindow/mainwindow.cpp
        mainwindow/mainwindow.h
        mainwindow/mainwindow.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(ImageConverter
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${PROJECT_WIDGETS}
        ${PROJECT_STYLES}
        ${PROJECT_PAGES}

    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET ImageConverter APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(ImageConverter SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(ImageConverter
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(ImageConverter PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Pdf)
target_include_directories(ImageConverter PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.ImageConverter)
endif()
set_target_properties(ImageConverter PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS ImageConverter
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ImageConverter)
endif()
