file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/base/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/base/*.c")
add_library(filterbase OBJECT ${SOURCES})
unset(SOURCES)
target_link_libraries(filterbase PUBLIC alutil)
set_target_properties(filterbase PROPERTIES POSITION_INDEPENDENT_CODE ON)

macro(defmod target source)
    add_library(${target} SHARED "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
    set_target_properties(${target} PROPERTIES PREFIX "")
    target_link_libraries(${target} PRIVATE filterbase alutil)
    target_defsym(${target} PRIVATE "dspmodule_cleanup" "alutil_quit")
endmacro()

defmod(dsplowpass "lowpass.c")