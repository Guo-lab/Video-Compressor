#include "algorithms/bilinear_downsample_algorithm.hpp"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

namespace vcompress {
namespace algorithm {

/**
 * @brief CUDA kernel for bilinear downsampling.
 */
__global__ void bilinearDownsampleKernel(const uint8_t *src, uint8_t *dst, int src_width, int src_height,
                                         int dst_width, int dst_height) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x < dst_width && y < dst_height) {
        float x_ratio = static_cast<float>(src_width - 1) / dst_width;
        float y_ratio = static_cast<float>(src_height - 1) / dst_height;

        float src_x = x * x_ratio;
        float src_y = y * y_ratio;
        int x_floor = static_cast<int>(src_x);
        int y_floor = static_cast<int>(src_y);
        int x_ceil = min(x_floor + 1, src_width - 1);
        int y_ceil = min(y_floor + 1, src_height - 1);
        float x_fraction = src_x - x_floor;
        float y_fraction = src_y - y_floor;

        for (int c = 0; c < 3; c++) {
            uint8_t p00 = src[(y_floor * src_width + x_floor) * 3 + c];
            uint8_t p01 = src[(y_floor * src_width + x_ceil) * 3 + c];
            uint8_t p10 = src[(y_ceil * src_width + x_floor) * 3 + c];
            uint8_t p11 = src[(y_ceil * src_width + x_ceil) * 3 + c];

            float top = p00 * (1.0f - x_fraction) + p01 * x_fraction;
            float bottom = p10 * (1.0f - x_fraction) + p11 * x_fraction;
            float result = top * (1.0f - y_fraction) + bottom * y_fraction;
            dst[(y * dst_width + x) * 3 + c] = static_cast<uint8_t>(result + 0.5f);
        }
    }
}

/**
 * @brief CUDA kernel for bilinear upsampling.
 */
__global__ void bilinearUpsampleKernel(const uint8_t *src, uint8_t *dst, int src_width, int src_height,
                                       int dst_width, int dst_height) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x < dst_width && y < dst_height) { // within destination image bounds
        float x_ratio = static_cast<float>(src_width - 1) / (dst_width - 1);
        float y_ratio = static_cast<float>(src_height - 1) / (dst_height - 1);

        float src_x = x * x_ratio;
        float src_y = y * y_ratio;
        int x_floor = static_cast<int>(src_x);
        int y_floor = static_cast<int>(src_y);
        int x_ceil = min(x_floor + 1, src_width - 1);
        int y_ceil = min(y_floor + 1, src_height - 1);
        float x_fraction = src_x - x_floor;
        float y_fraction = src_y - y_floor;

        for (int c = 0; c < 3; c++) {
            uint8_t p00 = src[(y_floor * src_width + x_floor) * 3 + c];
            uint8_t p01 = src[(y_floor * src_width + x_ceil) * 3 + c];
            uint8_t p10 = src[(y_ceil * src_width + x_floor) * 3 + c];
            uint8_t p11 = src[(y_ceil * src_width + x_ceil) * 3 + c];

            float top = p00 * (1.0f - x_fraction) + p01 * x_fraction;
            float bottom = p10 * (1.0f - x_fraction) + p11 * x_fraction;
            float result = top * (1.0f - y_fraction) + bottom * y_fraction;
            dst[(y * dst_width + x) * 3 + c] = static_cast<uint8_t>(result + 0.5f);
        }
    }
}

CudaBilinearDownsampleAlgorithm::CudaBilinearDownsampleAlgorithm() : BilinearDownsampleAlgorithm() {
    int deviceCount = 0;
    cudaError_t error = cudaGetDeviceCount(&deviceCount);
    if (error != cudaSuccess || deviceCount == 0) {
        std::cerr << "WARNING: No CUDA devices found! Falling back to CPU implementation." << std::endl;
        m_cuda_available = false;
    } else {
        m_cuda_available = true;
        cudaDeviceProp deviceProp;
        cudaGetDeviceProperties(&deviceProp, 0);
        std::cout << "Using CUDA Device: " << deviceProp.name << std::endl;
    }
}

CudaBilinearDownsampleAlgorithm::~CudaBilinearDownsampleAlgorithm() = default;

bool CudaBilinearDownsampleAlgorithm::initialize(const CompressionConfig &config) {
    bool result = BilinearDownsampleAlgorithm::initialize(config);
    if (m_cuda_available)
        std::cout << "Initialized CUDA bilinear downsample algorithm with factor: " << m_downsample_factor
                  << std::endl;
    else
        std::cout << "WARNING: CUDA not available, falling back to CPU implementation" << std::endl;
    return result;
}

/**
 * @brief Compresses a frame using CUDA bilinear downsampling when available.
 *  If CUDA is not available, it falls back to the CPU implementation.
 * @param frame The frame to compress.
 * @return The compressed frame data.
 */
std::vector<uint8_t> CudaBilinearDownsampleAlgorithm::compressFrame(const Frame &frame) {
    if (!m_cuda_available) return BilinearDownsampleAlgorithm::compressFrame(frame);

    auto start_time = std::chrono::high_resolution_clock::now();

    int original_width = frame.width;
    int original_height = frame.height;
    int target_width = original_width / m_downsample_factor;
    int target_height = original_height / m_downsample_factor;
    std::vector<uint8_t> downsampled(target_width * target_height * 3);
    cudaDownsampleBilinear(frame.data.data(), downsampled.data(), original_width, original_height,
                           target_width, target_height);

    double original_size = original_width * original_height * 3;
    double compressed_size = target_width * target_height * 3;
    double ratio = original_size / compressed_size;

    m_stats.frames_compressed++;
    m_stats.average_compression_ratio =
        ((m_stats.average_compression_ratio * (m_stats.frames_compressed - 1)) + ratio) /
        m_stats.frames_compressed;

    // Create compressed data format: | width (4) | height (4) | raw pixel data |
    std::vector<uint8_t> compressed_data(METADATA_BYTES + downsampled.size());
    std::memcpy(compressed_data.data(), &original_width, WIDTH_BYTES);
    std::memcpy(compressed_data.data() + WIDTH_BYTES, &original_height, HEIGHT_BYTES);
    std::memcpy(compressed_data.data() + METADATA_BYTES, downsampled.data(), downsampled.size());

    auto end_time = std::chrono::high_resolution_clock::now();
    double elapsed_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    m_stats.total_compression_time_ms += elapsed_ms;

    return compressed_data;
}

/**
 * @brief Decompresses a frame using CUDA bilinear upsampling when available.
 * If CUDA is not available, it falls back to the CPU implementation.
 * @param compressed_data The compressed data to decompress.
 * @return The decompressed frame.
 */
Frame CudaBilinearDownsampleAlgorithm::decompressFrame(const std::vector<uint8_t> &compressed_data) {
    if (!m_cuda_available) return BilinearDownsampleAlgorithm::decompressFrame(compressed_data);

    auto start_time = std::chrono::high_resolution_clock::now();

    int original_width, original_height;
    std::memcpy(&original_width, compressed_data.data(), WIDTH_BYTES);
    std::memcpy(&original_height, compressed_data.data() + WIDTH_BYTES, HEIGHT_BYTES);

    int downsampled_width = original_width / m_downsample_factor;
    int downsampled_height = original_height / m_downsample_factor;
    const uint8_t *downsampled_data = compressed_data.data() + METADATA_BYTES;
    std::vector<uint8_t> upsampled(original_width * original_height * 3);

    cudaUpsampleBilinear(downsampled_data, upsampled.data(), downsampled_width, downsampled_height,
                         original_width, original_height);

    Frame decompressed_frame(original_width, original_height);
    decompressed_frame.data = std::move(upsampled);
    decompressed_frame.type = KEY_FRAME;

    m_stats.frames_decompressed++;

    auto end_time = std::chrono::high_resolution_clock::now();
    double elapsed_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    m_stats.total_decompression_time_ms += elapsed_ms;

    return decompressed_frame;
}

/**
 * @brief CUDA version for bilinear downsampling.
 *  (i) Allocate device memory for source and destination images.
 *  (ii) Copy source image to device.
 *  (iii) Launch kernel for downsampling.
 *  (iv) Check for kernel errors and synchronize; copy result back to host.
 */
void CudaBilinearDownsampleAlgorithm::cudaDownsampleBilinear(const uint8_t *src, uint8_t *dst, int src_width,
                                                             int src_height, int dst_width, int dst_height) {
    uint8_t *d_src = nullptr, *d_dst = nullptr;
    auto cudaCleanup = [&]() {
        if (d_src) cudaFree(d_src);
        if (d_dst) cudaFree(d_dst);
    };
    size_t src_size = src_width * src_height * 3;
    size_t dst_size = dst_width * dst_height * 3;

    cudaError_t error;
    if ((error = cudaMalloc(&d_src, src_size)) != cudaSuccess) {
        std::cerr << "Source allocation failed: " << cudaGetErrorString(error) << std::endl;
        return;
    }
    if ((error = cudaMalloc(&d_dst, dst_size)) != cudaSuccess) {
        std::cerr << "Destination allocation failed: " << cudaGetErrorString(error) << std::endl;
        cudaCleanup();
        return;
    }

    if ((error = cudaMemcpy(d_src, src, src_size, cudaMemcpyHostToDevice)) != cudaSuccess) {
        std::cerr << "Host->device copy failed: " << cudaGetErrorString(error) << std::endl;
        cudaCleanup();
        return;
    }

    // Launch downsampling kernel
    dim3 blockSize(BLOCK_SIZE, BLOCK_SIZE);
    dim3 gridSize((dst_width + blockSize.x - 1) / blockSize.x, (dst_height + blockSize.y - 1) / blockSize.y);
    bilinearDownsampleKernel<<<gridSize, blockSize>>>(d_src, d_dst, src_width, src_height, dst_width,
                                                      dst_height);

    if ((error = cudaGetLastError()) != cudaSuccess || (error = cudaDeviceSynchronize()) != cudaSuccess) {
        std::cerr << "Kernel execution failed: " << cudaGetErrorString(error) << std::endl;
        cudaCleanup();
        return;
    }

    if ((error = cudaMemcpy(dst, d_dst, dst_size, cudaMemcpyDeviceToHost)) != cudaSuccess) {
        std::cerr << "Device->host copy failed: " << cudaGetErrorString(error) << std::endl;
    }
    cudaCleanup();
}

/**
 * @brief CUDA version for bilinear upsampling.
 *  (i) Allocate device memory for source and destination images.
 *  (ii) Copy source image to device.
 *  (iii) Launch kernel for upsampling.
 *  (iv) Check for kernel errors and synchronize; copy result back to host.
 *  (v) Free device memory.
 */
void CudaBilinearDownsampleAlgorithm::cudaUpsampleBilinear(const uint8_t *src, uint8_t *dst, int src_width,
                                                           int src_height, int dst_width, int dst_height) {
    uint8_t *d_src = nullptr, *d_dst = nullptr;
    size_t src_size = src_width * src_height * 3;
    size_t dst_size = dst_width * dst_height * 3;
    cudaError_t error;
    auto cudaCleanup = [&]() {
        if (d_src) cudaFree(d_src);
        if (d_dst) cudaFree(d_dst);
    };

    if ((error = cudaMalloc(&d_src, src_size)) != cudaSuccess) { // Allocate source buffer
        std::cerr << "Source allocation failed: " << cudaGetErrorString(error) << std::endl;
        return;
    }
    if ((error = cudaMalloc(&d_dst, dst_size)) != cudaSuccess) { // Allocate destination buffer
        std::cerr << "Destination allocation failed: " << cudaGetErrorString(error) << std::endl;
        cudaCleanup();
        return;
    }

    if ((error = cudaMemcpy(d_src, src, src_size, cudaMemcpyHostToDevice)) != cudaSuccess) {
        std::cerr << "Host->device copy failed: " << cudaGetErrorString(error) << std::endl;
        cudaCleanup();
        return;
    }

    dim3 blockSize(BLOCK_SIZE, BLOCK_SIZE);
    dim3 gridSize((dst_width + blockSize.x - 1) / blockSize.x, (dst_height + blockSize.y - 1) / blockSize.y);
    bilinearUpsampleKernel<<<gridSize, blockSize>>>(d_src, d_dst, src_width, src_height, dst_width,
                                                    dst_height);

    if ((error = cudaGetLastError()) != cudaSuccess ||
        (error = cudaDeviceSynchronize()) != cudaSuccess) { // Check for kernel errors and synchronize
        std::cerr << "Kernel execution failed: " << cudaGetErrorString(error) << std::endl;
        cudaCleanup();
        return;
    }

    if ((error = cudaMemcpy(dst, d_dst, dst_size, cudaMemcpyDeviceToHost)) != cudaSuccess) {
        std::cerr << "Device->host copy failed: " << cudaGetErrorString(error) << std::endl;
    }
    cudaCleanup();
}

} // namespace algorithm
} // namespace vcompress