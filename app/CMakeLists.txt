set(IZANAGI_RESOURCES
  ${CMAKE_SOURCE_DIR}/resources/izanagi.qrc
)

add_executable(izanagi
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
  ${IZANAGI_RESOURCES}
)

target_link_libraries(izanagi
  PRIVATE
    eleutheria_core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Core
)

target_compile_features(izanagi PRIVATE cxx_std_17)

if(WIN32)
  set_target_properties(izanagi PROPERTIES WIN32_EXECUTABLE TRUE)
elseif(APPLE)
  set(IZANAGI_MAC_ICON "${CMAKE_SOURCE_DIR}/resources/icons/izanagi-1024.png")
  set_target_properties(izanagi PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/macos/Info.plist.in"
    MACOSX_BUNDLE_BUNDLE_NAME "Izanagi"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.nebbiearcane.izanagi"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
  )
  add_custom_command(TARGET izanagi POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:izanagi>/Contents/Resources"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${IZANAGI_MAC_ICON}"
            "$<TARGET_BUNDLE_DIR:izanagi>/Contents/Resources/izanagi.png"
    COMMENT "Copia icona Izanagi nel bundle macOS"
  )
endif()

set_target_properties(izanagi PROPERTIES OUTPUT_NAME "izanagi")
