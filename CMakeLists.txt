cmake_minimum_required(VERSION 3.23)
project(dspaleffects LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(dspeff0 SHARED "src/main.c")
set_target_properties(dspeff0 PROPERTIES PREFIX "")

find_package(OpenAL REQUIRED)
target_link_libraries(dspeff0 PUBLIC OpenAL::OpenAL)
