cmake_minimum_required(VERSION 3.10.0)
project(Image_quilting VERSION 0.1.0 LANGUAGES C CXX)

# Find OpenCV package
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# Add executable
add_executable(quilting quilting.cpp)

# Link OpenCV libraries
target_link_libraries(quilting ${OpenCV_LIBS})