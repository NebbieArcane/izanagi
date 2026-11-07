add_library(eleutheria_core STATIC
  src/token_io.cpp
  src/catalog.cpp
  src/area_io.cpp
  src/source_blocks.cpp
  src/world.cpp
  src/workspace.cpp
)

target_include_directories(eleutheria_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(eleutheria_core PUBLIC cxx_std_17)

if(MSVC)
  target_compile_options(eleutheria_core PRIVATE /W4)
else()
  target_compile_options(eleutheria_core PRIVATE -Wall -Wextra -Wpedantic)
endif()
