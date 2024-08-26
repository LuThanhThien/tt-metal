cmake_minimum_required(VERSION 3.15)

project(ttsim)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)


# file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cpp/*.cpp)

# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/cpp)

add_library(ttsim INTERFACE)


# set_target_properties(ttsim PROPERTIES
#	ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
# )

target_include_directories(ttsim INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/ttsim/cpp
)


