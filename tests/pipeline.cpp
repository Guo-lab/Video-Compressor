#include "test.hpp"

// Use FFmpeg to extract audio from input video
bool extractAudio(const std::string &inputVideo, const std::string &outputAudio) {
    std::string cmd = "ffmpeg -i \"" + inputVideo + "\" -vn -acodec copy \"" + outputAudio + "\" -y";
    std::cout << "Extracting audio: " << cmd << std::endl;
    int result = std::system(cmd.c_str());
    return result == 0;
}

// Use FFmpeg to combine processed video with the original audio
bool combineVideoAudio(const std::string &videoFile, const std::string &audioFile,
                       const std::string &outputFile) {
    std::string cmd = "ffmpeg -i \"" + videoFile + "\" -i \"" + audioFile +
                      "\" -c:v copy -c:a aac -map 0:v:0 -map 1:a:0 \"" + outputFile + "\" -y";
    std::cout << "Combining video and audio: " << cmd << std::endl;
    int result = std::system(cmd.c_str());
    return result == 0;
}

int pipeline_main(int argc, char **argv) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <input_video> <output_video>" << std::endl;
        return -1;
    }

    std::string inputPath = argv[1];
    std::string outputPath = argv[2];
    std::string tempAudioPath = "../output_videos/temp_audio.aac";
    std::string tempVideoPath = "../output_videos/temp_processed_video.mp4";
    if (!extractAudio(inputPath, tempAudioPath)) {
        std::cerr << "Failed to extract audio from input video" << std::endl;
        return -1;
    }

    cv::VideoCapture cap(inputPath);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open input video: " << inputPath << std::endl;
        return -1;
    }

    int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    double fps = cap.get(cv::CAP_PROP_FPS);
    int fourcc = cv::VideoWriter::fourcc('a', 'v', 'c', 'i');

    std::cout << "Input video: " << width << "x" << height << " @ " << fps << " fps" << std::endl;

    cv::VideoWriter writer;
    writer.open(tempVideoPath, fourcc, fps, cv::Size(width, height));
    if (!writer.isOpened()) {
        std::cerr << "Error: Could not create output video: " << tempVideoPath << std::endl;
        return -1;
    }

    cv::Mat frame;
    int frameCount = 0;
    while (cap.read(frame)) {
        writer.write(frame);
        frameCount++;
        if (frameCount % 30 == 0) {
            std::cout << "Processed " << frameCount << " frames..." << std::endl;
        }
    }

    cap.release();
    writer.release();
    std::cout << "Completed processing " << frameCount << " frames." << std::endl;

    if (!combineVideoAudio(tempVideoPath, tempAudioPath, outputPath)) {
        std::cerr << "Failed to combine video and audio" << std::endl;
        return -1;
    }
    std::remove(tempAudioPath.c_str());
    std::remove(tempVideoPath.c_str());

    return 0;
}
