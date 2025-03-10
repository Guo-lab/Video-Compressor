#include "utils/audio.hpp"

namespace vcompress {
namespace utils {

// Use FFmpeg to extract audio from input video
bool extractAudio(const std::string &inputVideo, const std::string &outputAudio) {
    std::string cmd =
        "ffmpeg -i \"" + inputVideo + "\" -vn -acodec copy \"" + outputAudio + "\" -y -loglevel error";
    std::cout << "Extracting audio: " << cmd << std::endl;
    int result = std::system(cmd.c_str());
    return result == 0;
}

// Use FFmpeg to combine processed video with the original audio
bool combineVideoAudio(const std::string &videoFile, const std::string &audioFile,
                       const std::string &outputFile) {
    std::string cmd = "ffmpeg -i \"" + videoFile + "\" -i \"" + audioFile +
                      "\" -c:v copy -c:a aac -map 0:v:0 -map 1:a:0 \"" + outputFile + "\" -y -loglevel error";
    std::cout << "Combining video and audio: " << cmd << std::endl;
    int result = std::system(cmd.c_str());
    return result == 0;
}

} // namespace utils
} // namespace vcompress