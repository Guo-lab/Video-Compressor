#include "algorithms/cv_downsample_algorithm.hpp"
#include <chrono>
#include <iostream>
#include <sstream>

namespace vcompress {
namespace algorithm {

CVDownsampleAlgorithm::CVDownsampleAlgorithm() {
    m_stats.frames_compressed = 0;
    m_stats.frames_decompressed = 0;
    m_stats.average_compression_ratio = 0.0;
    m_stats.total_compression_time_ms = 0.0;
    m_stats.total_decompression_time_ms = 0.0;
    m_downsample_factor = 2;
}

/// Reset the algorithm state to initial conditions
void CVDownsampleAlgorithm::reset() {
    m_stats.frames_compressed = 0;
    m_stats.frames_decompressed = 0;
    m_stats.average_compression_ratio = 0.0;
    m_stats.total_compression_time_ms = 0.0;
    m_stats.total_decompression_time_ms = 0.0;
}

CVDownsampleAlgorithm::~CVDownsampleAlgorithm() = default;

/**
 * @brief Initialize the downsample algorithm with configuration
 *  Higher quality settings will result in less downsampling. Calculate factor (2-4) based on quality (1-100)
 * @param config The configuration settings for this algorithm
 */
bool CVDownsampleAlgorithm::initialize(const CompressionConfig &config) {
    m_config = config;
    m_downsample_factor = 4 - (m_config.quality / 50);
    m_downsample_factor = std::max(2, std::min(4, m_downsample_factor));
    std::cout << "Initialized downsample algorithm with factor: " << m_downsample_factor << std::endl;
    return true;
}

/**
 * @brief Compress a video frame. Downsample the image by a factor of 2 or 4; with the help of OpenCV.
 * @param frame The input video frame to compress
 * @return std::vector<uint8_t> The compressed data
 */
std::vector<uint8_t> CVDownsampleAlgorithm::compressFrame(const Frame &frame) {
    auto start_time = std::chrono::high_resolution_clock::now();

    cv::Mat original_mat = frameToMat(frame);
    int original_width = original_mat.cols;
    int original_height = original_mat.rows;

    // Downsample the image
    cv::Mat downsampled_mat;
    cv::resize(original_mat, downsampled_mat,
               cv::Size(original_width / m_downsample_factor, original_height / m_downsample_factor), 0, 0,
               cv::INTER_AREA);

    updateCompressionStats(original_mat, downsampled_mat);

    // Create compressed data: | width (4) | height (4) | raw pixel data |
    std::vector<uint8_t> compressed_data(METADATA_BYTES +
                                         downsampled_mat.total() * downsampled_mat.elemSize());
    copyMatToBuffer(downsampled_mat, original_width, original_height, compressed_data.data());

    auto end_time = std::chrono::high_resolution_clock::now();
    double elapsed_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    m_stats.total_compression_time_ms += elapsed_ms;

    return compressed_data;
}

/**
 * @brief Decompress a video frame. Upsample the image back to the original resolution.
 *  Extract metadata and pixel data from the compressed data buffer, then upsample the image back to the
 * original.
 */
Frame CVDownsampleAlgorithm::decompressFrame(const std::vector<uint8_t> &compressed_data) {
    auto start_time = std::chrono::high_resolution_clock::now();

    int original_width, original_height;
    cv::Mat downsampled_mat =
        copyBufferToMat(compressed_data.data(), original_width, original_height, compressed_data.size());

    // Upsample back to original resolution and convert back to Frame
    cv::Mat upsampled_mat;
    cv::resize(downsampled_mat, upsampled_mat, cv::Size(original_width, original_height), 0, 0,
               cv::INTER_LINEAR);
    Frame decompressed_frame = matToFrame(upsampled_mat);
    m_stats.frames_decompressed++;

    auto end_time = std::chrono::high_resolution_clock::now();
    double elapsed_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    m_stats.total_decompression_time_ms += elapsed_ms;

    return decompressed_frame;
}

/**
 * @brief Print the m_stats and related information about the downsample algorithm
 */
std::string CVDownsampleAlgorithm::getStats() const {
    std::stringstream ss;
    ss << "CVDownsample Algorithm Statistics:" << std::endl
       << "  OpenCV Downsample factor: " << m_downsample_factor << std::endl
       << "  Frames compressed: " << m_stats.frames_compressed << std::endl
       << "  Frames decompressed: " << m_stats.frames_decompressed << std::endl
       << "  Average compression ratio: " << m_stats.average_compression_ratio << ":1" << std::endl;

    if (m_stats.frames_compressed > 0) {
        ss << "  Average compression time: "
           << (m_stats.total_compression_time_ms / m_stats.frames_compressed) << " ms" << std::endl;
    }
    if (m_stats.frames_decompressed > 0) {
        ss << "  Average decompression time: "
           << (m_stats.total_decompression_time_ms / m_stats.frames_decompressed) << " ms" << std::endl;
    }

    return ss.str();
}

/**
 * @brief Convert a Frame to an OpenCV Mat
 *  With 8-bit unsigned values (8U) and 3 channels (RGB), copy all pixel values from our custom Frame format
 * to the OpenCV Mat.
 */
cv::Mat CVDownsampleAlgorithm::frameToMat(const Frame &frame) {
    cv::Mat mat(frame.height, frame.width, CV_8UC3);
    std::memcpy(mat.data, frame.data.data(), frame.data.size());
    return mat;
}

/**
 * @brief Update the compression statistics based on the original and compressed images
 */
void CVDownsampleAlgorithm::updateCompressionStats(const cv::Mat &original, const cv::Mat &compressed) {
    double original_size = original.total() * original.elemSize();
    double compressed_size = compressed.total() * compressed.elemSize();
    double ratio = original_size / compressed_size;

    m_stats.frames_compressed++;
    m_stats.average_compression_ratio =
        ((m_stats.average_compression_ratio * (m_stats.frames_compressed - 1)) + ratio) /
        m_stats.frames_compressed;
}

/**
 * @brief Copy pixel data from an OpenCV Mat to the Compressed data buffer
 *  This function is used to copy pixel data from an OpenCV Mat to a buffer in the format required by the
 * compression algorithm. It handles both continuous and non-continuous matrices.
 */
void CVDownsampleAlgorithm::copyMatToBuffer(const cv::Mat &mat, int w, int h, uint8_t *buffer) {
    // Store width and height
    std::memcpy(buffer, &w, WIDTH_BYTES);
    std::memcpy(buffer + WIDTH_BYTES, &h, HEIGHT_BYTES);
    // Store pixel data
    if (mat.isContinuous()) {
        std::memcpy(buffer + METADATA_BYTES, mat.data, mat.total() * mat.elemSize());
    } else {
        size_t row_bytes = mat.cols * mat.elemSize();
        for (int i = 0; i < mat.rows; ++i) {
            std::memcpy(buffer + METADATA_BYTES + i * row_bytes, mat.ptr(i), row_bytes);
        }
    }
}

/**
 * @brief Convert an OpenCV Mat back to a Frame; not considering the timestamp or type here.
 */
Frame CVDownsampleAlgorithm::matToFrame(const cv::Mat &mat) {
    Frame frame;
    frame.width = mat.cols;
    frame.height = mat.rows;
    frame.timestamp = 0;
    frame.data.resize(mat.total() * mat.elemSize());
    std::memcpy(frame.data.data(), mat.data, frame.data.size());
    return frame;
}

/// @brief Copy pixel data from the compressed data buffer to an OpenCV Mat
cv::Mat CVDownsampleAlgorithm::copyBufferToMat(const uint8_t *buffer, int &w, int &h, size_t bufferSize) {
    std::memcpy(&w, buffer, WIDTH_BYTES);
    std::memcpy(&h, buffer + WIDTH_BYTES, HEIGHT_BYTES);

    int downsampled_w = w / m_downsample_factor;
    int downsampled_h = h / m_downsample_factor;
    cv::Mat downsampled_mat(downsampled_h, downsampled_w, CV_8UC3);
    std::memcpy(downsampled_mat.data, buffer + METADATA_BYTES, bufferSize - METADATA_BYTES);
    return downsampled_mat;
}

} // namespace algorithm
} // namespace vcompress