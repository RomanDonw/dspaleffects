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

add_library(alutil OBJECT "src/alutil/general.c")
target_link_libraries(alutil PUBLIC OpenAL::OpenAL)
target_include_directories(alutil PUBLIC "src/")
set_target_properties(alutil PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ========================

add_library(jsonutil OBJECT "src/jsonutil/jsonutil.c")
target_link_libraries(jsonutil PUBLIC json-c::json-c)
target_include_directories(jsonutil PUBLIC "src/")
set_target_properties(jsonutil PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ========================

set(CMAKE_CURRENT_SOURCE_DIR "src/effects/")
include("src/effects/CMakeLists.txt")
set(CMAKE_CURRENT_SOURCE_DIR "src/filters/")
include("src/filters/CMakeLists.txt")