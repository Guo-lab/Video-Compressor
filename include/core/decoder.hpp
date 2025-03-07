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

/// @brief Configuration for video decoding
struct DecoderConfig {
    std::string compressedDataPath = "data.vcomp";
    std::string tempVideoPath = "temp_processed_video.mp4";
    std::string tempAudioPath = "temp_audio.aac";
    std::string inputPath;      // Input compressed video file path
    std::string outputPath;     // Output decompressed video file path
    std::string algorithmName;  // Decompression algorithm to use
    int quality = 75;           // Quality setting (may affect some algorithms)
    bool keepAudio = true;      // Whether to preserve audio
    bool keepTempFiles = false; // Whether to keep temporary files

    DecoderConfig() = default;
    DecoderConfig(const std::string &input, const std::string &output, const std::string &algo, int q,
                  bool keepAudio, bool keepTemp)
        : inputPath(input), outputPath(output), algorithmName(algo), quality(q), keepAudio(keepAudio),
          keepTempFiles(keepTemp) {}
};

/**
 * @brief Video decoder using decompression algorithms
 *
 * This class coordinates the video decompression process,
 * using FileReader, FileWriter, and a decompression algorithm.
 */
class VideoDecoder {
  public:
    /**
     * @brief Constructor
     */
    VideoDecoder();

    /**
     * @brief Destructor
     */
    ~VideoDecoder();

    /**
     * @brief Configure the decoder
     *
     * @param config Configuration settings
     * @return true if configuration was successful, false otherwise
     */
    bool configure(const DecoderConfig &config);

    /**
     * @brief Execute the decoding process
     *
     * @return true if decoding was successful, false otherwise
     */
    bool decode();

    /**
     * @brief Get decoding statistics
     *
     * @return String containing statistics
     */
    std::string getStats() const;

  private:
    /// Configuration
    DecoderConfig m_config;

    /// Components
    std::unique_ptr<algorithm::BaseCompressionAlgorithm> m_algorithm;
    std::unique_ptr<utils::FileWriter> m_fileWriter;
    std::unique_ptr<utils::CompressedFormat> m_compressedFormat;

    /// Statistics
    struct {
        int framesProcessed;
        int64_t totalInputSize;
        int64_t totalOutputSize;
        double averageTimePerFrame;
        double totalProcessingTime;
    } m_stats;

    // Helper methods
    bool createAlgorithm();
    bool processVideo();
    bool combineVideoWithAudio(const std::string &videoFile, const std::string &audioFile,
                               const std::string &outputFile);
};

} // namespace core
} // namespace vcompress
