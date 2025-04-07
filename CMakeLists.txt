cmake_minimum_required(VERSION 3.10)
project(GraphVTK)

# Set the output directory for executables to the project root.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

add_executable(GraphVTK draw3D.cpp)
target_link_libraries(GraphVTK ${VTK_LIBRARIES})