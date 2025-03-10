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
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


add_compile_options(
    -Wall
    -Wextra
    -O2
)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Werror")


#############################
#    External Libraries
#############################
find_package(OpenCV)
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
message(STATUS "OpenCV include directories: ${OpenCV_INCLUDE_DIRS}")

find_package(CUDA QUIET)
if(CUDA_FOUND)
    message(STATUS "CUDA found: ${CUDA_VERSION}")
    message(STATUS "CUDA libraries: ${CUDA_LIBRARIES}")
    message(STATUS "CUDA include directories: ${CUDA_INCLUDE_DIRS}")
    option(USE_CUDA "Build with CUDA support" ON)
else()
    option(USE_CUDA "Build with CUDA support" OFF)
    message(STATUS "CUDA not found, building CPU-only version")
endif()


#############################
#        Source Files
#############################
if(USE_CUDA)
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    set(CMAKE_CUDA_EXTENSIONS OFF)
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Xcudafe --display_error_number")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --compiler-options -Wno-error=deprecated-declarations -Wno-deprecated-gpu-targets")

    find_package(CUDAToolkit REQUIRED)
    file(GLOB_RECURSE CUDA_SOURCES "src/*.cu")

    message(STATUS "CUDA source files: ${CUDA_SOURCES}")
endif()

if(USE_CUDA)
    set(CMAKE_CUDA_ARCHITECTURES "50;60;70")
endif()

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
if(USE_CUDA)
    add_library(
        video_compressor_lib STATIC ${LIB_SOURCES} ${CUDA_SOURCES})

    target_include_directories(video_compressor_lib 
        PUBLIC 
            include 
            src
            ${OpenCV_INCLUDE_DIRS}
            ${CUDA_INCLUDE_DIRS})

    target_link_libraries(video_compressor_lib 
        PUBLIC ${OpenCV_LIBS} CUDA::cudart)

    target_compile_definitions(video_compressor_lib 
        PUBLIC USE_CUDA=1)

else()
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
            
    target_compile_definitions(video_compressor_lib 
        PRIVATE USE_CUDA=0)
endif()

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

if(USE_CUDA)
    # Create separate lib for CPU-only version to avoid definition conflicts
    add_library(video_compressor_lib_cpu 
        STATIC ${LIB_SOURCES})

    target_include_directories(video_compressor_lib_cpu 
        PUBLIC include src ${OpenCV_INCLUDE_DIRS})

    target_link_libraries(video_compressor_lib_cpu 
        PUBLIC ${OpenCV_LIBS})

    target_compile_definitions(video_compressor_lib_cpu 
        PRIVATE USE_CUDA=0)
    
    # Link CPU executable to CPU-only lib
    add_executable(video_compressor_cpu ${MAIN_SOURCE})
    target_link_libraries(video_compressor_cpu 
        PRIVATE video_compressor_lib_cpu)

    message(STATUS "CPU-only executable target name: video_compressor_cpu")
endif()


#############################
#  Submodule Configuration
#############################
# Add subdirectories for tests and examples
add_subdirectory(tests)


