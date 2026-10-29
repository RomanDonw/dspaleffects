option(BUILD_COMPLEX_EFFECTS "Build complex effects" ON)

macro(defmod target source) # define OpenAL effect module.
    add_library(${target} SHARED "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
    set_target_properties(${target} PROPERTIES PREFIX "")
    target_link_libraries(${target} PRIVATE alutil)
endmacro()

if(BUILD_COMPLEX_EFFECTS)
    defmod(dspoldradio "complex/oldradio.c")
    target_defsym(dspoldradio PRIVATE "dspmodule_cleanup" "alutil_quit")
endif()

# ========================

add_library(effectbase OBJECT "${CMAKE_CURRENT_SOURCE_DIR}/basic/base/base.c")
target_link_libraries(effectbase PUBLIC alutil jsonutil)
set_target_properties(effectbase PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(effectutil OBJECT "${CMAKE_CURRENT_SOURCE_DIR}/basic/util/util.c")
set_target_properties(effectutil PROPERTIES POSITION_INDEPENDENT_CODE ON)

macro(defsmod target source) # define simple (basic) module.
    defmod(${target} "basic/${source}")
    target_link_libraries(${target} PRIVATE effectbase jsonutil)
    target_defsym(${target} PRIVATE "dspmodule_cleanup" "alutil_quit")
endmacro()

macro(defumod target source) # define simple (basic) module with util library.
    defsmod(${target} ${source})
    target_link_libraries(${target} PRIVATE effectutil)
endmacro()

defumod(dspeaxreverb "eaxreverb.c")
defumod(dspchorus "chorus.c")
defumod(dspringmod "ringmod.c")
defumod(dspflanger "flanger.c")
defsmod(dspdistortion "distortion.c")
defumod(dspfreqshift "freqshift.c")