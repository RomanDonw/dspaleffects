cmake_minimum_required(VERSION 3.23)
project(dspaleffects LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

macro(target_defsym target name value)
    target_link_options(${target} PRIVATE "-Wl,--defsym=${name}=${value}")
endmacro()


file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/albase/*.h" "src/albase/*.c")
add_library(dspalbase OBJECT ${SOURCES})
unset(SOURCES)
find_package(OpenAL REQUIRED)
target_link_libraries(dspalbase PUBLIC OpenAL::OpenAL)
set_target_properties(dspalbase PROPERTIES POSITION_INDEPENDENT_CODE ON)

file(GLOB SOURCES CONFIGURE_DEPENDS "src/main.c" "src/jsonutil.*")
add_library(dspeff0 SHARED ${SOURCES})
set_target_properties(dspeff0 PROPERTIES PREFIX "")
unset(SOURCES)
find_package(json-c CONFIG REQUIRED)
target_link_libraries(dspeff0 PRIVATE dspalbase json-c::json-c)
target_defsym(dspeff0 "dspmodule_cleanup" "albase_quit")
