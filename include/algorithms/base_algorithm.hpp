#pragma once

#include <memory>
#include <string>
#include <vector>

namespace vcompress {
namespace algorithm {

// Frame data structure
struct Frame {
    int width;
    int height;
    std::vector<uint8_t> data;
    int timestamp;

    // Additional metadata can be added here
};

// Configuration for the algorithm
struct CompressionConfig {
    int quality;
    int target_bitrate;
    // Other parameters
};

// Base class for all compression algorithms
class BaseCompressionAlgorithm {
  public:
    virtual ~BaseCompressionAlgorithm() = default;

    // Initialize the algorithm with configuration
    virtual bool initialize(const CompressionConfig &config) = 0;

    // Compress a single frame
    virtual std::vector<uint8_t> compressFrame(const Frame &frame) = 0;

    // Decompress a frame
    virtual Frame decompressFrame(const std::vector<uint8_t> &compressed_data) = 0;

    // Get the name of the algorithm
    virtual std::string getName() const = 0;

    // Get algorithm-specific statistics
    virtual std::string getStats() const = 0;
};

// Factory to create algorithm instances
class AlgorithmFactory {
  public:
    // Get the list of available algorithms
    static std::vector<std::string> getAvailableAlgorithms();

    // Create an instance of an algorithm by name
    static std::unique_ptr<BaseCompressionAlgorithm> createAlgorithm(const std::string &name);
};

} // namespace algorithm
} // namespace vcompress