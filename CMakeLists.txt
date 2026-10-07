cmake_minimum_required(VERSION 3.23)
project(dspaleffects LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

macro(target_defsym target level name value)
    target_link_options(${target} ${level} "-Wl,--defsym=${name}=${value}")
endmacro()

find_package(json-c CONFIG REQUIRED)
find_package(OpenAL REQUIRED)

# ========================

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/alutil/*.h" "src/alutil/*.c")
add_library(alutil OBJECT ${SOURCES})
unset(SOURCES)
target_link_libraries(alutil PUBLIC OpenAL::OpenAL)
target_include_directories(alutil PUBLIC "src/")
set_target_properties(alutil PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ========================

file(GLOB SOURCES CONFIGURE_DEPENDS "src/jsonutil/*.h" "src/jsonutil/*.c")
add_library(jsonutil OBJECT ${SOURCES})
unset(SOURCES)
target_link_libraries(jsonutil PUBLIC json-c::json-c)
target_include_directories(jsonutil PUBLIC "src/")
set_target_properties(jsonutil PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ========================

macro(defmod target source)
    add_library(${target} SHARED ${source})
    set_target_properties(${target} PROPERTIES PREFIX "")
endmacro()

defmod(dspoldradio "src/complex/oldradio.c")
target_link_libraries(dspoldradio PRIVATE alutil)
target_defsym(dspoldradio PRIVATE "dspmodule_cleanup" "alutil_quit")

# ========================

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/basic/base/*.h" "src/basic/base/*.c")
add_library(effectbase OBJECT ${SOURCES})
unset(SOURCES)
target_link_libraries(effectbase PUBLIC alutil jsonutil)
set_target_properties(effectbase PROPERTIES POSITION_INDEPENDENT_CODE ON)

macro(defsimplemod target source)
    defmod(${target} "src/basic/${source}")
    target_link_libraries(${target} PRIVATE effectbase alutil jsonutil)
    target_defsym(${target} PRIVATE "dspmodule_cleanup" "alutil_quit")
endmacro()

defsimplemod(dspeaxreverb "eaxreverb.c")
defsimplemod(dspchorus "chorus.c")
defsimplemod(dspringmod "ringmod.c")
defsimplemod(dspflanger "flanger.c")
defsimplemod(dspdistortion "distortion.c")
defsimplemod(dspfreqshift "freqshift.c")