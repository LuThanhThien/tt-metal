project(SingleMatmul)

cmake_minimum_required(VERSION 3.15)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Automatically collect all .cpp files in the current source directory
file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# Add the includes
set(TT_METAL_HOME $ENV{TT_METAL_HOME})
include_directories(
    ${TT_METAL_HOME}/dev/ttsim/cpp
    ${TT_METAL_HOME}
)

foreach (TEST_SRC ${SOURCES})
    get_filename_component(TEST_TARGET ${TEST_SRC} NAME_WE)
    add_executable(${TEST_TARGET} ${TEST_SRC} )
    set_target_properties(${TEST_TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${SUBDIR})
    list(APPEND SOURCE_TARGETS ${TEST_TARGET})
endforeach()

set(SOURCE_TARGETS "${SOURCE_TARGETS}" PARENT_SCOPE)

# Add the custom target
add_custom_target(SingleMatmul DEPENDS ${SOURCE_TARGETS})
