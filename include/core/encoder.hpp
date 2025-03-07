#pragma once

#include "algorithms/base_algorithm.hpp"
#include "utils/audio.hpp"
#include "utils/compressed_format.hpp"
#include "utils/file_reader.hpp"
#include "utils/file_writer.hpp"
#include <memory>
#include <string>

namespace vcompress {
namespace core {

/// @brief Configuration for video encoding
struct EncoderConfig {
    std::string compressedDataPath = "data.vcomp";
    std::string tempVideoPath = "temp_processed_video.mp4";
    std::string tempAudioPath = "temp_audio.aac";
    std::string inputPath;             // Input video file path
    std::string outputPath;            // Output video file path
    std::string algorithmName;         // Compression algorithm to use
    int quality = 75;                  // Quality setting (1-100)
    int bitrate = 0;                   // Target bitrate in kbps (0 = variable)
    int keyFrameInterval = 30;         // Number of frames between key frames
    bool visualizeCompression = false; // Whether to show the compressed frames directly
    bool keepAudio = true;             // Whether to preserve audio
    bool keepTempFiles = false;        // Whether to keep temporary files

    EncoderConfig() = default;
    EncoderConfig(const std::string &input, const std::string &output, const std::string &algo, int q, int b,
                  int k, bool vis, bool keepAudio, bool keepTemp)
        : inputPath(input), outputPath(output), algorithmName(algo), quality(q), bitrate(b),
          keyFrameInterval(k), visualizeCompression(vis), keepAudio(keepAudio), keepTempFiles(keepTemp) {}
};

/**
 * @brief Video encoder using compression algorithms
 *
 * This class coordinates the video compression process,
 * using FileReader, FileWriter, and a compression algorithm.
 */
class VideoEncoder {
  public:
    /**
     * @brief Constructor
     */
    VideoEncoder();

    /**
     * @brief Destructor
     */
    ~VideoEncoder();

    /**
     * @brief Configure the encoder
     *
     * @param config Configuration settings
     * @return true if configuration was successful, false otherwise
     */
    bool configure(const EncoderConfig &config);

    /**
     * @brief Execute the encoding process
     *
     * @return true if encoding was successful, false otherwise
     */
    bool encode();

    /**
     * @brief Get encoding statistics
     *
     * @return String containing statistics
     */
    std::string getStats() const;

  private:
    /// Configuration
    EncoderConfig m_config;

    /// Components
    std::unique_ptr<algorithm::BaseCompressionAlgorithm> m_algorithm;
    std::unique_ptr<utils::FileReader> m_fileReader;
    std::unique_ptr<utils::CompressedFormat> m_compressedFormat;

    /// Statistics
    struct {
        int framesProcessed;
        int64_t totalInputSize;
        int64_t totalOutputSize;
        double compressionRatio;
        double averageTimePerFrame;
        double totalProcessingTime;
    } m_stats;

    /// Helper methods
    bool createAlgorithm();
    bool extractAudioFromVideo(const std::string &inputVideo, const std::string &outputAudio);
    bool processVideo(const std::string &inputVideo, const std::string &outputVideo);
};

} // namespace core
} // namespace vcompress