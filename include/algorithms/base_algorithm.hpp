#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace vcompress {
namespace algorithm {

/// Enums
// Video Compression uses both independent frames (key frames/I-frames) and dependent frames (delta
// frames/P-frames) to reduce the amount of data needed to represent a video sequence.
// Key frames are complete frames that can be decoded independently, while delta frames contain only the
// changes from the previous frame.
// This allows for efficient storage compression and transmission of video data.
enum FrameType { KEY_FRAME, DELTA_FRAME };

/// Structs
// Frame: Represents a single video frame with all necessary metadata
// With which the frame data can be consistent.
struct Frame {
    int width;
    int height;
    std::vector<uint8_t> data;
    int timestamp;
    FrameType type;
    // Constructors
    Frame() : width(0), height(0), timestamp(0), type(KEY_FRAME) {}
    Frame(int w, int h, const std::vector<uint8_t> &d, int ts, FrameType t)
        : width(w), height(h), data(d), timestamp(ts), type(t) {}
    Frame(int w, int h) : width(w), height(h), timestamp(0), type(KEY_FRAME) {}
};

// Configuration Settings for Video Compression (to control quality vs. size tradeoffs)
struct CompressionConfig {
    int quality;
    int target_bitrate;
    int key_frame_interval;
    // Constructors
    CompressionConfig() : quality(75), target_bitrate(0), key_frame_interval(30) {}
    CompressionConfig(int q, int bitrate, int kfi)
        : quality(q), target_bitrate(bitrate), key_frame_interval(kfi) {}
};

// Error Handling for Compression Algorithms (to report specific error conditions)
// Not used for now.
struct CompressionError {
    bool has_error;
    std::string message;
    // Constructors
    CompressionError() : has_error(false) {}
    CompressionError(const std::string &msg) : has_error(true), message(msg) {}
};

/// Class
// Base class: Defines the interface that all compression algorithms must implement
class BaseCompressionAlgorithm {
  public:
    /// Ensure proper cleanup when derived objects are deleted through a base class pointer
    virtual ~BaseCompressionAlgorithm() = default;

    /// Initialize the algorithm with configuration
    virtual bool initialize(const CompressionConfig &config) = 0;

    /// Compress a video frame:
    /// Takes an uncompressed video frame as input, processes it to reduce its data size,
    /// and returns a compressed representation
    virtual std::vector<uint8_t> compressFrame(const Frame &frame) = 0;

    /// Decompress a video frame:
    /// Takes the compressed data as input, reconstructs an approximation of the original frame,
    /// and returns a decompressed frame that can be displayed or further processed
    /// The compressed format produced by compressFrame can be correctly interpreted by decompressFrame.
    /// And the compression/decompression cycle preserves as much visual quality as possible.
    virtual Frame decompressFrame(const std::vector<uint8_t> &compressed_data) = 0;

    /// Get the name of the algorithm
    virtual std::string getAlgorithmName() const = 0;

    /// Get the algorithm-specific statistics/performance metrics
    virtual std::string getStats() const = 0;

    /// Get the last error that occurred
    virtual CompressionError getLastError() const = 0;

    /// Reset the algorithm state
    virtual void reset() = 0;
};

// Factory to create algorithm instances (Decoupling the creation)
class AlgorithmFactory {
  public:
    /// Function pointer type for algorithm creation
    /// CreatorFunction represents any function that returns a std::unique_ptr<BaseCompressionAlgorithm> and
    /// takes no parameters.
    typedef std::unique_ptr<BaseCompressionAlgorithm> (*CreatorFunction)();

    /// Register a new algorithm
    static bool registerAlgorithm(const std::string &name, CreatorFunction creator);

    /// Unregister an algorithm
    static bool unregisterAlgorithm(const std::string &name);

    /// Get the list of available algorithms
    static std::vector<std::string> getAvailableAlgorithms();

    /// Create an instance of an algorithm by name
    static std::unique_ptr<BaseCompressionAlgorithm> createAlgorithm(const std::string &name);

    /// Check if an algorithm is available
    static bool isAlgorithmAvailable(const std::string &name);

  private:
    // Map of algorithm names to creator functions
    static std::unordered_map<std::string, CreatorFunction> m_algorithm_creators;
};

} // namespace algorithm
} // namespace vcompress