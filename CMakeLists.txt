cmake_minimum_required(VERSION 3.21)
project(EleutheriaNebbie VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

option(ELEUTHERIA_BUILD_TESTS "Build smoke tests" ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core)

add_subdirectory(core)
add_subdirectory(app)

if(ELEUTHERIA_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

install(TARGETS eleutheria RUNTIME DESTINATION bin BUNDLE DESTINATION .)
install(DIRECTORY catalogs/ DESTINATION share/eleutheria/catalogs)
install(DIRECTORY sample/ DESTINATION share/eleutheria/sample)
