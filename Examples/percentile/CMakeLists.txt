cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(percentile)

# Find Qt5 Core 
find_package(Qt5 COMPONENTS Core REQUIRED)

# Find the QAlgorithm library
find_package(QAlgorithm REQUIRED)

# Include QAlgorithm's directories to the search path
include_directories(${QAlgorithm_INCLUDE_DIRS})

# Enables automatic moc generation
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Create the executable
add_executable(percentile main.cpp)

target_link_libraries(percentile ${QAlgorithm_LIBRARIES} Qt5::Core)

# Add C++14 support to the project
set_property(TARGET percentile PROPERTY CXX_STANDARD 17)
