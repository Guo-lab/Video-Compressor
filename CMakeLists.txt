cmake_minimum_required(VERSION 3.14)


#############################
#   Project Configuration
#############################
project(VideoCompressor VERSION 0.1.0 LANGUAGES CXX)



#############################
#   Compiler Configuration
#############################
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(
    -Wall
    -Wextra
    -O2
    -Werror
)



#############################
#    External Libraries
#############################
find_package(OpenCV REQUIRED)
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
message(STATUS "OpenCV include directories: ${OpenCV_INCLUDE_DIRS}")



#############################
#        Source Files
#############################
# Add all source files in the src folder and its subfolders
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB MAIN_SOURCE "src/main.cpp")

# Directly set LIB_SOURCES without main.cpp
set(LIB_SOURCES ${SOURCES})
list(FILTER LIB_SOURCES EXCLUDE REGEX "src/main.cpp")
message(STATUS "SOURCES: ${SOURCES}")
message(STATUS "MAIN_SOURCE: ${MAIN_SOURCE}")
message(STATUS "LIB_SOURCES: ${LIB_SOURCES}")



#############################
#       Build Targets
#############################
add_library(
    video_compressor_lib STATIC ${LIB_SOURCES})
target_include_directories(video_compressor_lib 
    PUBLIC 
        include 
        src
        ${OpenCV_INCLUDE_DIRS})
target_link_libraries(video_compressor_lib 
    PUBLIC 
        ${OpenCV_LIBS})
message(STATUS "Lib target name: video_compressor_lib")



#############################
#  Executable Configuration
#############################
# Create the executable
add_executable(
    video_compressor ${MAIN_SOURCE})

target_link_libraries(video_compressor 
    PRIVATE 
        video_compressor_lib)
message(STATUS "Executable target name: video_compressor")



#############################
#  Submodule Configuration
#############################
# Add subdirectories for tests and examples
add_subdirectory(tests)


