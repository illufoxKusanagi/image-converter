cmake_minimum_required(VERSION 3.16)

project(ImageConverter VERSION 1.2.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Pdf Concurrent Svg)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Pdf Concurrent Svg)

add_subdirectory(ui-kit)

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

set(PROJECT_PAGES
        pages/main_page.cpp
        pages/main_page.h
        pages/pdf_page.cpp
        pages/pdf_page.h
        pages/pdf/merge_pdf_panel.cpp
        pages/pdf/merge_pdf_panel.h
        pages/pdf/split_pdf_panel.cpp
        pages/pdf/split_pdf_panel.h
        pages/gallery_page.cpp
        pages/gallery_page.h
)

set(PROJECT_CONTROLLERS
        controllers/pdf/pdf_utils.cpp
        controllers/pdf/pdf_utils.h
        controllers/pdf/pdf_tool_controller.cpp
        controllers/pdf/pdf_tool_controller.h
        controllers/pdf/compress_controller.cpp
        controllers/pdf/compress_controller.h
        controllers/pdf/merge_controller.cpp
        controllers/pdf/merge_controller.h
        controllers/pdf/split_controller.cpp
        controllers/pdf/split_controller.h
        controllers/pdf/organize_controller.cpp
        controllers/pdf/organize_controller.h
        controllers/pdf/pdf_to_images_controller.cpp
        controllers/pdf/pdf_to_images_controller.h
        controllers/pdf/images_to_pdf_controller.cpp
        controllers/pdf/images_to_pdf_controller.h
        controllers/pdf/grayscale_controller.cpp
        controllers/pdf/grayscale_controller.h
        controllers/pdf/markdown_pdf_controller.cpp
        controllers/pdf/markdown_pdf_controller.h
        controllers/image/svg_convert_controller.cpp
        controllers/image/svg_convert_controller.h
        controllers/image/ico_export_controller.cpp
        controllers/image/ico_export_controller.h
)

set(PROJECT_SOURCES
        main.cpp
        mainwindow/mainwindow.cpp
        mainwindow/mainwindow.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(ImageConverter
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${PROJECT_WIDGETS}
        ${PROJECT_STYLES}
        ${PROJECT_PAGES}
        ${PROJECT_CONTROLLERS}
    )
else()
    if(ANDROID)
        add_library(ImageConverter SHARED
            ${PROJECT_SOURCES}
            ${PROJECT_WIDGETS}
            ${PROJECT_STYLES}
            ${PROJECT_PAGES}
            ${PROJECT_CONTROLLERS}
        )
    else()
        add_executable(ImageConverter
            ${PROJECT_SOURCES}
            ${PROJECT_WIDGETS}
            ${PROJECT_STYLES}
            ${PROJECT_PAGES}
            ${PROJECT_CONTROLLERS}
        )
    endif()
endif()

target_link_libraries(ImageConverter PRIVATE ui_kit Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Pdf Qt${QT_VERSION_MAJOR}::Concurrent Qt${QT_VERSION_MAJOR}::Svg)
target_include_directories(ImageConverter PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

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

if(UNIX AND NOT APPLE)
    install(FILES resources/image-converter.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES resources/icons/icon-logo.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps
        RENAME image-converter.png)
endif()

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ImageConverter)
endif()

# CPack Packaging Configuration
set(CPACK_PACKAGE_NAME "image-converter")
set(CPACK_PACKAGE_VENDOR "Illufox Kusanagi")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Fast offline image converter and PDF compressor built with Qt 6")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

# Debian (.deb)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Illufox Kusanagi")
set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
# Ghostscript is invoked as a subprocess, so shared-library scanning misses it.
set(CPACK_DEBIAN_PACKAGE_DEPENDS "ghostscript")

# RPM (.rpm)
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Graphics")
set(CPACK_RPM_PACKAGE_AUTOREQPROV ON)
set(CPACK_RPM_PACKAGE_REQUIRES "ghostscript")

include(CPack)
