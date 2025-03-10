# Video-Compressor

- ### Run the following command to build the project:
    `cd build` 
    `cmake ..`  
    `make`  

- ### Run the following command to compress a video file:

    `./video_compressor <input_video> <output_video> -a <algorithm> --keep-temp`

    where `<input_video>` is the path to the video file to be compressed, `<output_video>` is the path to the compressed video file, `<algorithm>` is the algorithm to be used for compression, and `--keep-temp` is an optional flag to keep the temporary files generated during the compression process.

- ### More usage information can be found by running:

    `./video_compressor --help`


- ### Run the following command to run the tests:

    `./tests/test_video_compressor  ../input_videos/SABRINA.mp4 ../output_videos/output_test.mp4`


## Appendix
Before running the program, make sure you have the following libraries installed:
- OpenCV  
    `sudo apt install libopencv-dev`    
    `pkg-config --modversion opencv4`  
- FFmpeg  
  `sudo apt update && sudo apt install ffmpeg -y`  
  `ffmpeg -version`  

- CMake
- GTest
- (Optional) CUDA `nvcc --version` 