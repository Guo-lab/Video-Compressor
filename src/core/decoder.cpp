#include "core/decoder.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sstream>

namespace vcompress {
namespace core {

/// @brief Constructor
VideoDecoder::VideoDecoder()
    : m_fileWriter(std::make_unique<vcompress::utils::FileWriter>()),
      m_compressedFormat(std::make_unique<vcompress::utils::CompressedFormat>()) {

    m_stats.framesProcessed = 0;
    m_stats.totalInputSize = 0;
    m_stats.totalOutputSize = 0;
    m_stats.averageTimePerFrame = 0.0;
    m_stats.totalProcessingTime = 0.0;
}

/// @brief Destructor
VideoDecoder::~VideoDecoder() = default;

/// @brief Configure the decoder
bool VideoDecoder::configure(const DecoderConfig &config) {
    m_config = config;
    return createAlgorithm();
}

/// @brief Create the decompression algorithm
bool VideoDecoder::createAlgorithm() {
    m_algorithm = algorithm::AlgorithmFactory::createAlgorithm(m_config.algorithmName);
    if (!m_algorithm) {
        std::cerr << "Error: Failed to create algorithm: " << m_config.algorithmName << std::endl;
        return false;
    }

    algorithm::CompressionConfig algoConfig;
    algoConfig.quality = m_config.quality;
    if (!m_algorithm->initialize(algoConfig)) {
        std::cerr << "Error: Failed to initialize algorithm: " << m_config.algorithmName << std::endl;
        return false;
    }

    std::cout << "Created and initialized algorithm: " << m_algorithm->getAlgorithmName() << std::endl;
    return true;
}

/// @brief Execute the decoding process
bool VideoDecoder::decode() {
    auto startTime = std::chrono::high_resolution_clock::now();

    // Decoder: vcomp -> temp.mp4 (would be deleted afterwards) -> output.mp4
    if (m_config.keepAudio) {
        std::cout << "<iii> Processing video frames with decompression " << m_config.algorithmName
                  << " algorithm..." << std::endl;
        if (!processVideo()) {
            std::cerr << "Failed to process video frames" << std::endl;
            return false;
        }
        std::cout << "<iv> Combining processed video with original audio..." << std::endl;
        if (!combineVideoWithAudio(m_config.tempVideoPath, m_config.tempAudioPath, m_config.outputPath)) {
            std::cerr << "Failed to combine video and audio" << std::endl;
            return false;
        } else {
            std::remove(m_config.tempVideoPath.c_str());
        }
    } else {
        std::cout << "Processing video frames with " << m_config.algorithmName
                  << " algorithm without audio..." << std::endl;
        if (!processVideo()) {
            std::cerr << "Failed to process video frames" << std::endl;
            return false;
        } else {
            std::rename(m_config.tempVideoPath.c_str(), m_config.outputPath.c_str());
        }
    }

    if (!m_config.keepTempFiles) {
        if (m_config.keepAudio) {
            std::remove(m_config.tempAudioPath.c_str());
        }
        std::remove(m_config.compressedDataPath.c_str());
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    m_stats.totalProcessingTime = std::chrono::duration<double>(endTime - startTime).count();

    std::cout << "Video processing Decoding completed successfully!" << std::endl;
    std::cout << "Final Output saved to: " << m_config.outputPath << std::endl;
    std::cout << getStats() << std::endl;

    return true;
}

bool VideoDecoder::combineVideoWithAudio(const std::string &videoFile, const std::string &audioFile,
                                         const std::string &outputFile) {
    return vcompress::utils::combineVideoAudio(videoFile, audioFile, outputFile);
}

bool VideoDecoder::processVideo() {
    if (!m_compressedFormat->openForReading(m_config.compressedDataPath)) {
        std::cerr << "Error: Could not open compressed file: " << m_config.compressedDataPath << std::endl;
        return false;
    }
    int width = m_compressedFormat->getOriginalWidth();
    int height = m_compressedFormat->getOriginalHeight();
    double fps = m_compressedFormat->getOriginalFPS();

    // asume it is avc1 codec for mp4 file. Refactor codec selection into enum-based.
    int fourcc = cv::VideoWriter::fourcc('a', 'v', 'c', '1');

    std::cout << "Original video dimensions: " << width << "x" << height << std::endl;
    std::cout << "Original video FPS: " << fps << std::endl;

    if (!m_fileWriter->openFile(m_config.tempVideoPath, width, height, fps, fourcc)) {
        std::cerr << "Error: Could not create output video: " << m_config.tempVideoPath << std::endl;
        return false;
    }

    std::vector<uint8_t> compressedData;
    bool isKeyFrame;
    cv::Mat outputFrame;
    auto totalStartTime = std::chrono::high_resolution_clock::now();

    while (m_compressedFormat->readFrame(compressedData, isKeyFrame)) {
        auto frameStartTime = std::chrono::high_resolution_clock::now();

        m_stats.totalInputSize += compressedData.size();
        algorithm::Frame decompressedFrame = m_algorithm->decompressFrame(compressedData);
        m_stats.totalOutputSize += decompressedFrame.data.size();

        outputFrame = cv::Mat(decompressedFrame.height, decompressedFrame.width, CV_8UC3);
        std::memcpy(outputFrame.data, decompressedFrame.data.data(), decompressedFrame.data.size());

        m_fileWriter->writeFrame(outputFrame);

        auto frameEndTime = std::chrono::high_resolution_clock::now();
        double frameTime = std::chrono::duration<double, std::milli>(frameEndTime - frameStartTime).count();

        m_stats.framesProcessed++;
        m_stats.averageTimePerFrame =
            ((m_stats.averageTimePerFrame * (m_stats.framesProcessed - 1)) + frameTime) /
            m_stats.framesProcessed;
        if (m_stats.framesProcessed % 500 == 0)
            std::cout << "Decompressed " << m_stats.framesProcessed << " frames..." << std::endl;
    }

    auto totalEndTime = std::chrono::high_resolution_clock::now();
    m_stats.totalProcessingTime = std::chrono::duration<double>(totalEndTime - totalStartTime).count();

    m_compressedFormat->close();
    m_fileWriter->close();
    std::cout << "Completed decompressing " << m_stats.framesProcessed << " frames." << std::endl;

    return true;
}

/// @brief Get decoding statistics
std::string VideoDecoder::getStats() const {
    std::stringstream ss;

    ss << "Decoding Statistics:" << std::endl
       << "  Algorithm: " << m_config.algorithmName << std::endl
       << "  Frames processed: " << m_stats.framesProcessed << std::endl
       << "  Total input size: " << m_stats.totalInputSize << " bytes" << std::endl
       << "  Total output size: " << m_stats.totalOutputSize << " bytes" << std::endl
       << "  Average time per frame: " << m_stats.averageTimePerFrame << " ms" << std::endl
       << "  Total processing time: " << m_stats.totalProcessingTime << " seconds" << std::endl;
    if (m_algorithm) ss << "Algorithm Statistics:" << std::endl << m_algorithm->getStats();

    return ss.str();
}

} // namespace core
} // namespace vcompress