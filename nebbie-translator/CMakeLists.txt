find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(NEBBIE_TRANSLATE_ICON_MAC "${CMAKE_CURRENT_SOURCE_DIR}/icons/nebbie-translate.icns")
set(NEBBIE_TRANSLATE_ICON_WIN "${CMAKE_CURRENT_SOURCE_DIR}/icons/nebbie-translate.ico")

add_executable(nebbie-translate
    main.cpp
    app_config.cpp
    app_config.hpp
    translator_room_widget.cpp
    translator_room_widget.hpp
    translator_window.cpp
    translator_window.hpp
    nebbie-translate.qrc
    ${CMAKE_SOURCE_DIR}/nebbie-qt/app_i18n.cpp
    ${CMAKE_SOURCE_DIR}/nebbie-qt/app_i18n.hpp
    ${CMAKE_SOURCE_DIR}/nebbie-qt/app_theme.cpp
    ${CMAKE_SOURCE_DIR}/nebbie-qt/app_theme.hpp
    ${CMAKE_SOURCE_DIR}/nebbie-qt/release_update_checker.cpp
    ${CMAKE_SOURCE_DIR}/nebbie-qt/release_update_checker.hpp
    ${CMAKE_SOURCE_DIR}/nebbie-qt/release_update_util.cpp
    ${CMAKE_SOURCE_DIR}/nebbie-qt/release_update_util.hpp
    ${CMAKE_SOURCE_DIR}/nebbie-qt/mud_color_widgets.cpp
    ${CMAKE_SOURCE_DIR}/nebbie-qt/mud_color_widgets.hpp
    ${CMAKE_SOURCE_DIR}/nebbie-qt/mud_color_list_delegate.cpp
    ${CMAKE_SOURCE_DIR}/nebbie-qt/mud_color_list_delegate.hpp
    ${CMAKE_SOURCE_DIR}/nebbie-qt/mud_color_dialogs.cpp
    ${CMAKE_SOURCE_DIR}/nebbie-qt/mud_color_dialogs.hpp
    ${CMAKE_SOURCE_DIR}/nebbie-qt/validation_report_ui.cpp
    ${CMAKE_SOURCE_DIR}/nebbie-qt/validation_report_ui.hpp
)

if(APPLE AND EXISTS "${NEBBIE_TRANSLATE_ICON_MAC}")
    set_source_files_properties("${NEBBIE_TRANSLATE_ICON_MAC}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(nebbie-translate PRIVATE "${NEBBIE_TRANSLATE_ICON_MAC}")
endif()

if(WIN32 AND EXISTS "${NEBBIE_TRANSLATE_ICON_WIN}")
    target_sources(nebbie-translate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/nebbie-translate.rc")
endif()

target_include_directories(nebbie-translate PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/nebbie-qt"
    "${CMAKE_BINARY_DIR}/generated"
)

target_link_libraries(nebbie-translate PRIVATE Qt6::Widgets Qt6::Network nebbie-core)
target_compile_features(nebbie-translate PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(nebbie-translate PRIVATE "/FI${CMAKE_SOURCE_DIR}/nebbie-qt/msvc_qt_compat.hpp")
endif()

if(WIN32)
    set_target_properties(nebbie-translate PROPERTIES WIN32_EXECUTABLE TRUE OUTPUT_NAME cypher)
endif()

if(APPLE AND NEBBIE_MACOS_BUNDLE_TRANSLATOR)
    set_target_properties(nebbie-translate PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.nebbie.translate"
        MACOSX_BUNDLE_BUNDLE_NAME "Cypher — Nebbie Arcane room editor"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    )
    if(EXISTS "${NEBBIE_TRANSLATE_ICON_MAC}")
        set_target_properties(nebbie-translate PROPERTIES MACOSX_BUNDLE_ICON_FILE nebbie-translate)
    endif()
endif()

include(GNUInstallDirs)

if(NOT (APPLE AND NEBBIE_MACOS_BUNDLE_TRANSLATOR))
    install(TARGETS nebbie-translate RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    if(NOT WIN32)
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/nebbie-translate.desktop"
                DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
        foreach(_icon_size 16 22 24 32 48 64 128 256 512 1024)
            set(_icon_path "${CMAKE_CURRENT_SOURCE_DIR}/icons/hicolor/${_icon_size}x${_icon_size}/apps/nebbie-translate.png")
            if(EXISTS "${_icon_path}")
                install(FILES "${_icon_path}"
                        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/${_icon_size}x${_icon_size}/apps")
                install(FILES "${_icon_path}"
                        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/${_icon_size}x${_icon_size}/apps"
                        RENAME cypher.png)
            endif()
        endforeach()
    endif()
endif()

if(APPLE AND NEBBIE_MACOS_BUNDLE_TRANSLATOR)
    install(TARGETS nebbie-translate BUNDLE DESTINATION "/Applications")
endif()

if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${Qt6_DIR}/../../../bin" "${_qt_bin_dir}")
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET nebbie-translate POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" "$<TARGET_FILE:nebbie-translate>"
            COMMENT "Deploying Qt runtime for nebbie-translate"
        )
    endif()
endif()
