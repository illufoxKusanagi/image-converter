cmake_minimum_required(VERSION 3.16)

project(ImageConverter VERSION 1.4.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Pdf Concurrent Svg Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Pdf Concurrent Svg Network)
find_package(Qt${QT_VERSION_MAJOR} QUIET OPTIONAL_COMPONENTS LinguistTools)
if(TARGET Qt${QT_VERSION_MAJOR}::lrelease AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/translations/imageconverter_id.ts")
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/resources/translations/imageconverter_id.qm"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/resources/translations"
        COMMAND Qt${QT_VERSION_MAJOR}::lrelease
            "${CMAKE_CURRENT_SOURCE_DIR}/translations/imageconverter_id.ts"
            -qm "${CMAKE_CURRENT_SOURCE_DIR}/resources/translations/imageconverter_id.qm"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/translations/imageconverter_id.ts"
        COMMENT "Compiling translation imageconverter_id.qm"
        VERBATIM
    )
    add_custom_target(imageconverter_translations ALL
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/resources/translations/imageconverter_id.qm"
    )
endif()

add_subdirectory(ui-kit)

set(PROJECT_WIDGETS
        widgets/drop_file_widget.h
        widgets/drop_file_widget.cpp
        widgets/slider_widget.h
        widgets/slider_widget.cpp
)

set(PROJECT_STYLES
        resources/icons.qrc
)

set(PROJECT_PAGES
        pages/main_page.cpp
        pages/main_page.h
        pages/image/base64_panel.cpp
        pages/image/base64_panel.h
        pages/image/convert_image_panel.cpp
        pages/image/convert_image_panel.h
        pages/image/ico_packager_panel.cpp
        pages/image/ico_packager_panel.h
        pages/image/svg_options_panel.cpp
        pages/image/svg_options_panel.h
        pages/pdf_page.cpp
        pages/pdf_page.h
        pages/pdf/images_to_pdf_panel.cpp
        pages/pdf/images_to_pdf_panel.h
        pages/pdf/markdown_pdf_panel.cpp
        pages/pdf/markdown_pdf_panel.h
        pages/pdf/merge_pdf_panel.cpp
        pages/pdf/merge_pdf_panel.h
        pages/pdf/organize_pdf_panel.cpp
        pages/pdf/organize_pdf_panel.h
        pages/pdf/pdf_quality_panel.cpp
        pages/pdf/pdf_quality_panel.h
        pages/pdf/pdf_to_images_panel.cpp
        pages/pdf/pdf_to_images_panel.h
        pages/pdf/split_pdf_panel.cpp
        pages/pdf/split_pdf_panel.h
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
        controllers/image/image_convert_controller.cpp
        controllers/image/image_convert_controller.h
        controllers/image/svg_convert_controller.cpp
        controllers/image/svg_convert_controller.h
        controllers/image/ico_export_controller.cpp
        controllers/image/ico_export_controller.h
)

set(PROJECT_SOURCES
        main.cpp
        mainwindow/mainwindow.cpp
        mainwindow/mainwindow.h
        mainwindow/trakteer_support_button.cpp
        mainwindow/trakteer_support_button.h
        mainwindow/update_checker.cpp
        mainwindow/update_checker.h
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

target_link_libraries(ImageConverter PRIVATE ui_kit Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Pdf Qt${QT_VERSION_MAJOR}::Concurrent Qt${QT_VERSION_MAJOR}::Svg Qt${QT_VERSION_MAJOR}::Network)
target_compile_definitions(ImageConverter PRIVATE IMAGECONVERTER_VERSION="${PROJECT_VERSION}")
target_include_directories(ImageConverter PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(TARGET imageconverter_translations)
    add_dependencies(ImageConverter imageconverter_translations)
endif()

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
set(CPACK_RPM_PACKAGE_LICENSE "AGPL-3.0-or-later")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Graphics")
set(CPACK_RPM_PACKAGE_AUTOREQPROV ON)
set(CPACK_RPM_PACKAGE_REQUIRES "ghostscript")

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

include(CPack)
