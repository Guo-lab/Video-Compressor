#pragma once

#include "base_algorithm.hpp"
#include <opencv2/opencv.hpp>

namespace vcompress {
namespace algorithm {

/// @brief By reducing the spatial resolution of each frame.
class CVDownsampleAlgorithm : public BaseCompressionAlgorithm {
  public:
    CVDownsampleAlgorithm();
    ~CVDownsampleAlgorithm() override;

    bool initialize(const CompressionConfig &config) override;
    std::vector<uint8_t> compressFrame(const Frame &frame) override;
    Frame decompressFrame(const std::vector<uint8_t> &compressed_data) override;
    std::string getAlgorithmName() const override { return "CVDownsample"; }
    std::string getStats() const override;
    CompressionError getLastError() const override { return CompressionError(); }
    void reset() override;

    void updateCompressionStats(const cv::Mat &original, const cv::Mat &compressed);
    void copyMatToBuffer(const cv::Mat &mat, int w, int h, uint8_t *buffer);
    cv::Mat copyBufferToMat(const uint8_t *buffer, int &w, int &h, size_t bufferSize);

  private:
    // Shared by All instances
    static const size_t WIDTH_BYTES = 4;
    static const size_t HEIGHT_BYTES = 4;
    static const size_t METADATA_BYTES = WIDTH_BYTES + HEIGHT_BYTES;

    CompressionConfig m_config;
    CompressionError m_last_error;

    /// Downsampling factor - higher number means more compression
    /// 2 = half resolution, 4 = quarter resolution, etc.
    int m_downsample_factor;

    struct {
        int frames_compressed;
        int frames_decompressed;
        double average_compression_ratio;
        double total_compression_time_ms;
        double total_decompression_time_ms;
    } m_stats;

    /// Convert between our Frame struct and OpenCV's Mat class
    cv::Mat frameToMat(const Frame &frame);
    Frame matToFrame(const cv::Mat &mat);
};

} // namespace algorithm
} // namespace vcompress