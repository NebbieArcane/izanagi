add_executable(eleutheria_smoke
  smoke_test.cpp
)
target_link_libraries(eleutheria_smoke PRIVATE eleutheria_core)
add_test(NAME eleutheria_smoke COMMAND eleutheria_smoke
  ${CMAKE_SOURCE_DIR}/sample/B1)

add_executable(editor_regression
  editor_regression_test.cpp
)
target_link_libraries(editor_regression PRIVATE eleutheria_core)
target_compile_definitions(editor_regression PRIVATE
  IZANAGI_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

# Sample + alcune aree di produzione (se presenti accanto al repo).
set(_AREE_SRC "${CMAKE_SOURCE_DIR}/../Aree/src")
set(_REGRESSION_ARGS
  "${CMAKE_SOURCE_DIR}/sample/B1"
)
if(EXISTS "${_AREE_SRC}/acq/acq.ele")
  list(APPEND _REGRESSION_ARGS "${_AREE_SRC}/acq")
endif()
if(EXISTS "${_AREE_SRC}/myst/myst.ele")
  list(APPEND _REGRESSION_ARGS "${_AREE_SRC}/myst")
endif()
if(EXISTS "${_AREE_SRC}/hill_giant_/hill_giant_.ele")
  list(APPEND _REGRESSION_ARGS "${_AREE_SRC}/hill_giant_")
endif()
if(EXISTS "${_AREE_SRC}/lower_pyramid/lower_pyramid.ele")
  list(APPEND _REGRESSION_ARGS "${_AREE_SRC}/lower_pyramid")
endif()

add_test(NAME editor_regression COMMAND editor_regression ${_REGRESSION_ARGS})
