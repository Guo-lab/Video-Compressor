#pragma once

#include "base_algorithm.hpp"
#include <opencv2/opencv.hpp>

namespace vcompress {
namespace algorithm {

/// @brief By reducing the spatial resolution of each frame.
class BilinearDownsampleAlgorithm : public BaseCompressionAlgorithm {
  public:
    BilinearDownsampleAlgorithm();
    ~BilinearDownsampleAlgorithm() override;

    bool initialize(const CompressionConfig &config) override;
    std::vector<uint8_t> compressFrame(const Frame &frame) override;
    Frame decompressFrame(const std::vector<uint8_t> &compressed_data) override;
    std::string getAlgorithmName() const override { return "BilinearDownsample"; }
    std::string getStats() const override;
    CompressionError getLastError() const override { return CompressionError(); }
    void reset() override;

  protected:
    // Shared by All instances
    static const size_t WIDTH_BYTES = 4;
    static const size_t HEIGHT_BYTES = 4;
    static const size_t METADATA_BYTES = WIDTH_BYTES + HEIGHT_BYTES;

    CompressionConfig m_config;
    CompressionError m_last_error;
    int m_downsample_factor;

    struct {
        int frames_compressed;
        int frames_decompressed;
        double average_compression_ratio;
        double total_compression_time_ms;
        double total_decompression_time_ms;
    } m_stats;

    void downsampleBilinear(const uint8_t *src, uint8_t *dst, int src_width, int src_height, int dst_width,
                            int dst_height);

    void upsampleBilinear(const uint8_t *src, uint8_t *dst, int src_width, int src_height, int dst_width,
                          int dst_height);
};

/**
 * @brief CUDA-accelerated downsampling algorithm
 * This class provides a GPU-accelerated implementation of image downsampling
 * using bilinear interpolation with CUDA.
 */
class CudaBilinearDownsampleAlgorithm : public BilinearDownsampleAlgorithm {
  public:
    CudaBilinearDownsampleAlgorithm();
    ~CudaBilinearDownsampleAlgorithm() override;

    bool initialize(const CompressionConfig &config) override;
    std::vector<uint8_t> compressFrame(const Frame &frame) override;
    Frame decompressFrame(const std::vector<uint8_t> &compressed_data) override;
    std::string getAlgorithmName() const override { return "CudaBilinearDownsample"; }

  private:
    // CUDA-specific members and methods
    static const int BLOCK_SIZE = 16; // Block size for CUDA kernel (16x16 threads)
    bool m_cuda_available;
    void cudaDownsampleBilinear(const uint8_t *src, uint8_t *dst, int src_width, int src_height,
                                int dst_width, int dst_height);
    void cudaUpsampleBilinear(const uint8_t *src, uint8_t *dst, int src_width, int src_height, int dst_width,
                              int dst_height);
};

} // namespace algorithm
} // namespace vcompress