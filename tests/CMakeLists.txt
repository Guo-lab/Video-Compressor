add_executable(
    test_video_compressor test.cpp pipeline.cpp)

target_include_directories(
    test_video_compressor PUBLIC tests)

target_link_libraries(
    test_video_compressor PRIVATE video_compressor_lib)

