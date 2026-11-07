add_executable(eleutheria_smoke
  smoke_test.cpp
)
target_link_libraries(eleutheria_smoke PRIVATE eleutheria_core)
add_test(NAME eleutheria_smoke COMMAND eleutheria_smoke
  ${CMAKE_SOURCE_DIR}/sample/B1)
