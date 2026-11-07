set(ELEUTHERIA_RESOURCES
  ${CMAKE_SOURCE_DIR}/resources/eleutheria.qrc
)

add_executable(eleutheria
  main.cpp
  main_window.cpp
  main_window.hpp
  app_theme.cpp
  app_theme.hpp
  file_dialogs.cpp
  file_dialogs.hpp
  flag_editor.cpp
  flag_editor.hpp
  line_limited_edit.cpp
  line_limited_edit.hpp
  editors.cpp
  editors.hpp
  ${ELEUTHERIA_RESOURCES}
)

target_link_libraries(eleutheria
  PRIVATE
    eleutheria_core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Core
)

target_compile_features(eleutheria PRIVATE cxx_std_17)

if(WIN32)
  set_target_properties(eleutheria PROPERTIES WIN32_EXECUTABLE TRUE)
elseif(APPLE)
  set(ELEUTHERIA_MAC_ICON "${CMAKE_SOURCE_DIR}/resources/icons/eleutheria-1024.png")
  set_target_properties(eleutheria PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/macos/Info.plist.in"
    MACOSX_BUNDLE_BUNDLE_NAME "Eleutheria"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.nebbiearcane.eleutheria"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
  )
  # Su macOS si puo' convertire la PNG in .icns in post-build se iconutil e' disponibile.
  add_custom_command(TARGET eleutheria POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:eleutheria>/Contents/Resources"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ELEUTHERIA_MAC_ICON}"
            "$<TARGET_BUNDLE_DIR:eleutheria>/Contents/Resources/eleutheria.png"
    COMMENT "Copia icona Eleutheria nel bundle macOS"
  )
endif()

set_target_properties(eleutheria PROPERTIES OUTPUT_NAME "eleutheria")
