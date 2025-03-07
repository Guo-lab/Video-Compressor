#include "core/encoder.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>

namespace vcompress {
namespace core {

/// @brief Constructor
VideoEncoder::VideoEncoder()
    : m_fileReader(std::make_unique<vcompress::utils::FileReader>()),
      m_compressedFormat(std::make_unique<vcompress::utils::CompressedFormat>()) {

    m_stats.framesProcessed = 0;
    m_stats.totalInputSize = 0;
    m_stats.totalOutputSize = 0;
    m_stats.compressionRatio = 0.0;
    m_stats.averageTimePerFrame = 0.0;
    m_stats.totalProcessingTime = 0.0;
}

/// @brief Destructor
VideoEncoder::~VideoEncoder() = default;

/// @brief Configure the encoder
bool VideoEncoder::configure(const EncoderConfig &config) {
    m_config = config;
    return createAlgorithm();
}

/// @brief Create the compression algorithm
bool VideoEncoder::createAlgorithm() {
    m_algorithm = algorithm::AlgorithmFactory::createAlgorithm(m_config.algorithmName);
    if (!m_algorithm) {
        std::cerr << "Error: Failed to create algorithm: " << m_config.algorithmName << std::endl;
        return false;
    }

    algorithm::CompressionConfig algoConfig(m_config.quality, m_config.bitrate, m_config.keyFrameInterval);
    if (!m_algorithm->initialize(algoConfig)) {
        std::cerr << "Error: Failed to initialize algorithm: " << m_config.algorithmName << std::endl;
        return false;
    }

    std::cout << "Created and initialized algorithm: " << m_algorithm->getAlgorithmName() << std::endl;
    return true;
}

/// @brief Execute the encoding process; main processing pipeline- processVideo()
bool VideoEncoder::encode() {
    auto startTime = std::chrono::high_resolution_clock::now();

    if (m_config.keepAudio) {
        std::cout << "<i> Extracting audio from the original video..." << std::endl;
        if (!extractAudioFromVideo(m_config.inputPath, m_config.tempAudioPath)) {
            std::cerr << "Failed to extract audio from input video" << std::endl;
            return false;
        }
        std::cout << "<ii> Processing video frames with " << m_config.algorithmName << " algorithm..."
                  << std::endl;
        if (!processVideo(m_config.inputPath, m_config.compressedDataPath)) {
            std::cerr << "Failed to process video frames" << std::endl;
            return false;
        }
    } else {
        std::cout << "Processing video frames with " << m_config.algorithmName
                  << " algorithm without audio..." << std::endl;
        if (!processVideo(m_config.inputPath, m_config.compressedDataPath)) {
            std::cerr << "Failed to process video frames" << std::endl;
            return false;
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    m_stats.totalProcessingTime = std::chrono::duration<double>(endTime - startTime).count();

    std::cout << "Video processing Encoding completed successfully!" << std::endl;
    std::cout << "Compressed Data saved to: " << m_config.compressedDataPath << std::endl;
    std::cout << getStats() << std::endl;

    return true;
}

/// @brief Extract audio from the input video by the encoder
bool VideoEncoder::extractAudioFromVideo(const std::string &inputVideo, const std::string &outputAudio) {
    return vcompress::utils::extractAudio(inputVideo, outputAudio);
}

/// @brief Process video frames with the compression algorithm
bool VideoEncoder::processVideo(const std::string &inputVideo, const std::string &outputVideo) {
    if (!m_fileReader->openFile(inputVideo)) {
        std::cerr << "Error: Could not open input video: " << inputVideo << std::endl;
        return false;
    }

    int width = m_fileReader->getWidth();
    int height = m_fileReader->getHeight();
    double fps = m_fileReader->getFPS();
    uint16_t algorithmId = 1; // CVDownsample as 1 for now

    if (!m_compressedFormat->openForWriting(outputVideo, width, height, fps, algorithmId)) {
        std::cerr << "Error: Could not create output file: " << outputVideo << std::endl;
        return false;
    }

    cv::Mat frame;
    algorithm::Frame inputFrame;
    int frameCount = 0;
    bool isKeyFrame = true;
    auto totalStartTime = std::chrono::high_resolution_clock::now();

    while (m_fileReader->readNextFrame(frame)) {
        auto frameStartTime = std::chrono::high_resolution_clock::now();
        isKeyFrame = (frameCount % m_config.keyFrameInterval == 0);

        inputFrame.width = frame.cols;
        inputFrame.height = frame.rows;
        inputFrame.timestamp = frameCount;
        inputFrame.type = isKeyFrame ? algorithm::KEY_FRAME : algorithm::DELTA_FRAME;
        inputFrame.data.resize(frame.total() * frame.elemSize());
        std::memcpy(inputFrame.data.data(), frame.data, inputFrame.data.size());

        m_stats.totalInputSize += inputFrame.data.size();
        std::vector<uint8_t> compressed_data = m_algorithm->compressFrame(inputFrame);
        m_compressedFormat->writeFrame(compressed_data, isKeyFrame);
        m_stats.totalOutputSize += compressed_data.size();

        auto frameEndTime = std::chrono::high_resolution_clock::now();
        double frameTime = std::chrono::duration<double, std::milli>(frameEndTime - frameStartTime).count();

        m_stats.framesProcessed++;
        m_stats.averageTimePerFrame =
            ((m_stats.averageTimePerFrame * (m_stats.framesProcessed - 1)) + frameTime) /
            m_stats.framesProcessed;

        if (frameCount % 500 == 0) {
            std::cout << "Processed " << frameCount << " frames..." << std::endl;
        }
        frameCount++;
    }

    auto totalEndTime = std::chrono::high_resolution_clock::now();
    m_stats.totalProcessingTime = std::chrono::duration<double>(totalEndTime - totalStartTime).count();
    if (m_stats.totalInputSize > 0) {
        m_stats.compressionRatio = static_cast<double>(m_stats.totalInputSize) / m_stats.totalOutputSize;
    }

    m_fileReader->close();
    m_compressedFormat->close();
    std::cout << "Completed processing " << frameCount << " frames." << std::endl;

    return true;
}

/// @brief Get encoding statistics
std::string VideoEncoder::getStats() const {
    std::stringstream ss;

    ss << "Encoding Statistics:" << std::endl
       << "  Algorithm: " << m_config.algorithmName << std::endl
       << "  Frames processed: " << m_stats.framesProcessed << std::endl
       << "  Total input size: " << m_stats.totalInputSize << " bytes" << std::endl
       << "  Total output size: " << m_stats.totalOutputSize << " bytes" << std::endl
       << "  Compression ratio: " << m_stats.compressionRatio << ":1" << std::endl
       << "  Average time per frame: " << m_stats.averageTimePerFrame << " ms" << std::endl
       << "  Total processing time: " << m_stats.totalProcessingTime << " seconds" << std::endl;
    if (m_algorithm) ss << "Algorithm Statistics:" << std::endl << m_algorithm->getStats();

    return ss.str();
}

} // namespace core
} // namespace vcompress