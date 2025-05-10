cmake_minimum_required(VERSION 3.10.0)
project(Image_quilting)

# Find OpenCV package
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# Set the C++ standard
set(CMAKE_CXX_STANDARD 14)

# Ajouter les options de compilation
set(MY_COMPILE_OPTIONS "-Wall;-Wextra;-pedantic;-Wno-comment;-Wshadow;-Wnon-virtual-dtor;-Wpedantic;-Wconversion;-Wmisleading-indentation;-Wsign-conversion")

# Add executable
add_executable(quilting quilting.cpp)
target_link_libraries(quilting ${OpenCV_LIBS})
target_compile_options(quilting PRIVATE ${MY_COMPILE_OPTIONS})

# Add executable
add_executable(horizontal_quilting horizontal_quilting.cpp)
target_link_libraries(horizontal_quilting ${OpenCV_LIBS})
target_compile_options(horizontal_quilting PRIVATE ${MY_COMPILE_OPTIONS})