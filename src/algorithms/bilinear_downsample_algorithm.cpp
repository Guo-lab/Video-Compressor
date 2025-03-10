#include "algorithms/bilinear_downsample_algorithm.hpp"
#include <chrono>
#include <iostream>
#include <sstream>

namespace vcompress {
namespace algorithm {

BilinearDownsampleAlgorithm::BilinearDownsampleAlgorithm() {
    m_stats.frames_compressed = 0;
    m_stats.frames_decompressed = 0;
    m_stats.average_compression_ratio = 0.0;
    m_stats.total_compression_time_ms = 0.0;
    m_stats.total_decompression_time_ms = 0.0;
    m_downsample_factor = 2;
}

/// Reset the algorithm state to initial conditions
void BilinearDownsampleAlgorithm::reset() {
    m_stats.frames_compressed = 0;
    m_stats.frames_decompressed = 0;
    m_stats.average_compression_ratio = 0.0;
    m_stats.total_compression_time_ms = 0.0;
    m_stats.total_decompression_time_ms = 0.0;
}

BilinearDownsampleAlgorithm::~BilinearDownsampleAlgorithm() = default;

/**
 * @brief Initialize the downsample algorithm with configuration
 *  Higher quality settings will result in less downsampling. Calculate factor (2-4) based on quality (1-100)
 * @param config The configuration settings for this algorithm
 */
bool BilinearDownsampleAlgorithm::initialize(const CompressionConfig &config) {
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
std::vector<uint8_t> BilinearDownsampleAlgorithm::compressFrame(const Frame &frame) {
    auto start_time = std::chrono::high_resolution_clock::now();

    int original_width = frame.width;
    int original_height = frame.height;
    int target_width = original_width / m_downsample_factor;
    int target_height = original_height / m_downsample_factor;
    std::vector<uint8_t> downsampledBuffer(target_width * target_height * 3);

    downsampleBilinear(frame.data.data(), downsampledBuffer.data(), original_width, original_height,
                       target_width, target_height);

    double original_size = original_width * original_height * 3;
    double compressed_size = target_width * target_height * 3;
    double ratio = original_size / compressed_size;

    m_stats.frames_compressed++;
    m_stats.average_compression_ratio =
        ((m_stats.average_compression_ratio * (m_stats.frames_compressed - 1)) + ratio) /
        m_stats.frames_compressed;

    // Create compressed data format: | width (4) | height (4) | raw pixel data |
    std::vector<uint8_t> compressed_data(METADATA_BYTES + downsampledBuffer.size());
    std::memcpy(compressed_data.data(), &original_width, WIDTH_BYTES);
    std::memcpy(compressed_data.data() + WIDTH_BYTES, &original_height, HEIGHT_BYTES);
    std::memcpy(compressed_data.data() + METADATA_BYTES, downsampledBuffer.data(), downsampledBuffer.size());

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
Frame BilinearDownsampleAlgorithm::decompressFrame(const std::vector<uint8_t> &compressed_data) {
    auto start_time = std::chrono::high_resolution_clock::now();

    int original_width, original_height;
    std::memcpy(&original_width, compressed_data.data(), WIDTH_BYTES);
    std::memcpy(&original_height, compressed_data.data() + WIDTH_BYTES, HEIGHT_BYTES);

    int downsampled_width = original_width / m_downsample_factor;
    int downsampled_height = original_height / m_downsample_factor;
    std::vector<uint8_t> upsampledBuffer(original_width * original_height * 3);

    // Upsample back to original resolution and convert back to Frame
    upsampleBilinear(compressed_data.data() + METADATA_BYTES, upsampledBuffer.data(), downsampled_width,
                     downsampled_height, original_width, original_height);

    Frame decompressed_frame(original_width, original_height);
    decompressed_frame.data = std::move(upsampledBuffer);
    decompressed_frame.type = KEY_FRAME;

    m_stats.frames_decompressed++;

    auto end_time = std::chrono::high_resolution_clock::now();
    double elapsed_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    m_stats.total_decompression_time_ms += elapsed_ms;

    return decompressed_frame;
}

/**
 * @brief Print the m_stats and related information about the downsample algorithm
 */
std::string BilinearDownsampleAlgorithm::getStats() const {
    std::stringstream ss;
    ss << getAlgorithmName() << " Algorithm Statistics:" << std::endl
       << "  OpenCV Downsample factor: " << m_downsample_factor << std::endl
       << "  Frames compressed: " << m_stats.frames_compressed << std::endl
       << "  Frames decompressed: " << m_stats.frames_decompressed << std::endl
       << "  Average compression ratio: " << m_stats.average_compression_ratio << ":1" << std::endl;

    if (m_stats.frames_compressed > 0)
        ss << "  Average compression time: "
           << (m_stats.total_compression_time_ms / m_stats.frames_compressed) << " ms" << std::endl;

    if (m_stats.frames_decompressed > 0)
        ss << "  Average decompression time: "
           << (m_stats.total_decompression_time_ms / m_stats.frames_decompressed) << " ms" << std::endl;
    return ss.str();
}

/**
 * @brief Calculate the interpolation parameters for bilinear interpolation;
 *  Returns the floor, ceil, and fraction for a given position and ratio
 * @param pos The position in the destination image
 * @param ratio The scaling ratio
 * @param max_dim The maximum dimension (width or height) of the source image
 * @return std::tuple<int, int, float> The floor, ceil, and fraction of the source position
 *  The fraction denotes the distance between the target to the floor and ceil position. 0-1.
 *  This is the "inverse" of the interpolation weight.
 */
std::tuple<int, int, float> calculateInterpolationParams(float pos, float ratio, int max_dim) {
    float src_pos = pos * ratio;
    int floor = static_cast<int>(src_pos);
    int ceil = std::min(floor + 1, max_dim - 1);
    float fraction = src_pos - floor;
    return std::make_tuple(floor, ceil, fraction);
}

/**
 * @brief Get a pixel value at a specific position.
 *
 * @param src,src_width The source image data, The width of the source image
 * @param y,x The y-coordinate, x-coordinate of the pixel
 * @param c The color channel (0 for R, 1 for G, 2 for B)
 * @return uint8_t The pixel value at the specified position in the specified color channel
 */
uint8_t getPixelValue(const uint8_t *src, int src_width, int y, int x, int c) {
    return src[(y * src_width + x) * 3 + c];
}

/**
 * @brief Custom bilinear downsampling implementation
 *  Performs downsampling using bilinear interpolation
 *
 * @param src Source image data (RGB format)
 * @param dst Destination buffer for downsampled image
 * @param src_width Source image width
 * @param src_height Source image height
 * @param dst_width Destination image width
 * @param dst_height Destination image height
 */
void BilinearDownsampleAlgorithm::downsampleBilinear(const uint8_t *src, uint8_t *dst, int src_width,
                                                     int src_height, int dst_width, int dst_height) {
    // To properly access the full range of source pixels
    float x_ratio = static_cast<float>(src_width - 1) / dst_width;
    float y_ratio = static_cast<float>(src_height - 1) / dst_height;

    for (int y = 0; y < dst_height; y++) {
        auto [y_floor, y_ceil, y_fraction] = calculateInterpolationParams(y, y_ratio, src_height);
        for (int x = 0; x < dst_width; x++) {
            auto [x_floor, x_ceil, x_fraction] = calculateInterpolationParams(x, x_ratio, src_width);

            // For each color channel (RGB), get the four pixels for bilinear interpolation
            for (int c = 0; c < 3; c++) {
                uint8_t p00 = getPixelValue(src, src_width, y_floor, x_floor, c);
                uint8_t p01 = getPixelValue(src, src_width, y_floor, x_ceil, c);
                uint8_t p10 = getPixelValue(src, src_width, y_ceil, x_floor, c);
                uint8_t p11 = getPixelValue(src, src_width, y_ceil, x_ceil, c);

                float top = p00 * (1 - x_fraction) + p01 * x_fraction;
                float bottom = p10 * (1 - x_fraction) + p11 * x_fraction;
                float result = top * (1 - y_fraction) + bottom * y_fraction;

                // Store the final pixel value in the destination image
                dst[(y * dst_width + x) * 3 + c] = static_cast<uint8_t>(result + 0.5f);
            }
        }
    }
}

/**
 * @brief Custom bilinear upsampling implementation
 *  Performs upsampling using bilinear interpolation
 *
 * @param src Source image data (RGB format)
 * @param dst Destination buffer for upsampled image
 * @param src_width Source image width
 * @param src_height Source image height
 * @param dst_width Destination image width
 * @param dst_height Destination image height
 */
void BilinearDownsampleAlgorithm::upsampleBilinear(const uint8_t *src, uint8_t *dst, int src_width,
                                                   int src_height, int dst_width, int dst_height) {
    // From smaller to larger
    float x_ratio = static_cast<float>(src_width - 1) / (dst_width - 1);
    float y_ratio = static_cast<float>(src_height - 1) / (dst_height - 1);

    for (int y = 0; y < dst_height; y++) {
        auto [y_floor, y_ceil, y_fraction] = calculateInterpolationParams(y, y_ratio, src_height);
        for (int x = 0; x < dst_width; x++) {
            auto [x_floor, x_ceil, x_fraction] = calculateInterpolationParams(x, x_ratio, src_width);
            for (int c = 0; c < 3; c++) {
                uint8_t p00 = getPixelValue(src, src_width, y_floor, x_floor, c);
                uint8_t p01 = getPixelValue(src, src_width, y_floor, x_ceil, c);
                uint8_t p10 = getPixelValue(src, src_width, y_ceil, x_floor, c);
                uint8_t p11 = getPixelValue(src, src_width, y_ceil, x_ceil, c);
                float top = p00 * (1 - x_fraction) + p01 * x_fraction;
                float bottom = p10 * (1 - x_fraction) + p11 * x_fraction;
                float result = top * (1 - y_fraction) + bottom * y_fraction;
                dst[(y * dst_width + x) * 3 + c] = static_cast<uint8_t>(result + 0.5f);
            }
        }
    }
}

} // namespace algorithm
} // namespace vcompress