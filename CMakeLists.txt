cmake_minimum_required(VERSION 3.23)
project(dspaleffects LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.h" "src/*.c")
add_library(dspeff0 SHARED ${SOURCES})
set_target_properties(dspeff0 PROPERTIES PREFIX "")
unset(SOURCES)

find_package(OpenAL REQUIRED)
find_package(json-c CONFIG REQUIRED)
target_link_libraries(dspeff0 PUBLIC OpenAL::OpenAL json-c::json-c)