cmake_minimum_required(VERSION 3.23)
project(dspaleffects LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

macro(target_defsym target name value)
    target_link_options(${target} PRIVATE "-Wl,--defsym=${name}=${value}")
endmacro()

find_package(json-c CONFIG REQUIRED)
find_package(OpenAL REQUIRED)

# ========================

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/alutil/*.h" "src/alutil/*.c")
add_library(alutil OBJECT ${SOURCES})
unset(SOURCES)
target_link_libraries(alutil PUBLIC OpenAL::OpenAL)
set_target_properties(alutil PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ========================

file(GLOB SOURCES CONFIGURE_DEPENDS "src/jsonutil/*.h" "src/jsonutil/*.c")
add_library(jsonutil OBJECT ${SOURCES})
unset(SOURCES)
target_link_libraries(jsonutil PUBLIC json-c::json-c)
set_target_properties(jsonutil PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ========================

add_library(dspeaxreverb SHARED "src/eaxreverb.c")
set_target_properties(dspeaxreverb PROPERTIES PREFIX "")
target_link_libraries(dspeaxreverb PRIVATE alutil jsonutil)
target_defsym(dspeaxreverb "dspmodule_cleanup" "alutil_quit")

#add_library(dspchorus SHARED "src/chorus.c")
#set_target_properties(dspchorus PROPERTIES PREFIX "")
#target_link_libraries(dspchorus PRIVATE alutil jsonutil)
#target_defsym(dspchorus "dspmodule_cleanup" "alutil_quit")