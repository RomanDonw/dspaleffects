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
find_package(SampleRate REQUIRED)

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

macro(defmod target source)
    add_library(${target} SHARED ${source})
    set_target_properties(${target} PROPERTIES PREFIX "")
    target_link_libraries(${target} PRIVATE alutil jsonutil)
    target_defsym(${target} "dspmodule_cleanup" "alutil_quit")
endmacro()

defmod(dspeaxreverb "src/eaxreverb.c")
defmod(dspchorus "src/chorus.c")
defmod(dspoldradio "src/oldradio.c")
target_link_libraries(dspoldradio PUBLIC SampleRate::samplerate)