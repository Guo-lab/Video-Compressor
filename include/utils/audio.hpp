#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>

namespace vcompress {
namespace utils {

// Use FFmpeg to extract audio from input video
bool extractAudio(const std::string &inputVideo, const std::string &outputAudio);

// Use FFmpeg to combine processed video with the original audio
bool combineVideoAudio(const std::string &videoFile, const std::string &audioFile,
                       const std::string &outputFile);

} // namespace utils
} // namespace vcompress