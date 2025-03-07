#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace vcompress {
namespace utils {

/**
 * @brief A simple compressed video file format
 *
 * Minimal format specification:
 * - Header (14 bytes):
 *   - Original Width (4 bytes)
 *   - Original Height (4 bytes)
 *   - FPS (4 bytes, float)
 *   - Algorithm ID (2 bytes)
 *
 * - For each frame:
 *   - Frame type (1 byte) - 0: Key frame, 1: Delta frame
 *   - Frame size (4 bytes)
 *   - Compressed frame data (variable size)
 */
class CompressedFormat {
  public:
    /**
     * @brief Constructor
     */
    CompressedFormat() : m_isOpen(false), m_originalWidth(0), m_originalHeight(0), m_originalFPS(0.0) {}

    /**
     * @brief Destructor
     */
    ~CompressedFormat() { close(); }

    /**
     * @brief Opens a file for writing compressed data
     *
     * @param filename Output file path
     * @param width Original video width
     * @param height Original video height
     * @param fps Original video frame rate
     * @return true if file was opened successfully
     */
    bool openForWriting(const std::string &filename, int width, int height, double fps,
                        uint16_t algorithmId) {
        close();
        m_isWriteMode = true;

        m_file.open(filename, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!m_file.is_open()) return false;

        m_isOpen = true;
        m_originalWidth = width;
        m_originalHeight = height;
        m_originalFPS = fps;
        m_algorithmId = algorithmId;
        int32_t fps_int = static_cast<int32_t>(m_originalFPS * 1000);
        std::cout << "Opened compressed file: " << filename << std::endl;
        std::cout << "  Dimensions: " << width << "x" << height << std::endl;
        std::cout << "  FPS: " << fps_int << std::endl;
        std::cout << "  Algorithm ID: " << algorithmId << std::endl;
        m_file.write(reinterpret_cast<const char *>(&m_originalWidth), 4);
        m_file.write(reinterpret_cast<const char *>(&m_originalHeight), 4);
        m_file.write(reinterpret_cast<const char *>(&fps_int), 4);
        m_file.write(reinterpret_cast<const char *>(&m_algorithmId), 2);

        return !m_file.fail();
    }

    /**
     * @brief Opens a file for reading compressed data
     *
     * @param filename Input file path
     * @return true if file was opened successfully
     */
    bool openForReading(const std::string &filename) {
        close();
        m_isWriteMode = false;

        m_file.open(filename, std::ios::in | std::ios::binary);
        if (!m_file.is_open()) return false;
        m_isOpen = true;

        int32_t fps_int;
        m_file.read(reinterpret_cast<char *>(&m_originalWidth), 4);
        m_file.read(reinterpret_cast<char *>(&m_originalHeight), 4);
        m_file.read(reinterpret_cast<char *>(&fps_int), 4);
        m_file.read(reinterpret_cast<char *>(&m_algorithmId), 2);

        m_originalFPS = static_cast<double>(fps_int) / 1000.0;
        std::cout << "Opened compressed file: " << filename << std::endl;
        std::cout << "  Dimensions: " << m_originalWidth << "x" << m_originalHeight << std::endl;
        std::cout << "  FPS: " << m_originalFPS << std::endl;
        std::cout << "  Algorithm ID: " << m_algorithmId << std::endl;

        if (m_file.fail()) {
            close();
            return false;
        }
#ifdef DEBUG
        std::cout << "Opened compressed file: " << filename << std::endl;
#endif
        return true;
    }

    /**
     * @brief Writes a compressed frame to the file
     *
     * @param frameData Compressed frame data
     * @return true if frame was written successfully
     */
    bool writeFrame(const std::vector<uint8_t> &frameData, bool isKeyFrame) {
        if (!m_file.is_open() || !m_isWriteMode) return false;

        uint8_t frameType = isKeyFrame ? 0 : 1;
        uint32_t frameSize = static_cast<uint32_t>(frameData.size());

        std::vector<std::byte> batchBuffer(1 + 4 + frameData.size());
        auto [typeOffset, sizeOffset, dataOffset] = std::make_tuple(0, 1, 5);

        std::memcpy(batchBuffer.data() + typeOffset, &frameType, sizeof(frameType));
        std::memcpy(batchBuffer.data() + sizeOffset, &frameSize, sizeof(frameSize));
        std::memcpy(batchBuffer.data() + dataOffset, frameData.data(), frameData.size());
        m_file.write(reinterpret_cast<const char *>(batchBuffer.data()), batchBuffer.size());

        return !m_file.fail();
    }

    /**
     * @brief Reads the next compressed frame from the file
     *
     * @param frameData Vector to store the compressed frame data
     * @return true if a frame was successfully read
     */
    bool readFrame(std::vector<uint8_t> &frameData, bool &isKeyFrame) {
        if (!m_file.is_open() || m_isWriteMode) return false;

        constexpr long HEADER_SIZE = 5;
        std::array<std::byte, HEADER_SIZE> header;
        m_file.read(reinterpret_cast<char *>(header.data()), header.size());
        if (m_file.eof() && m_file.gcount() == 0) return false;
        if (m_file.gcount() < HEADER_SIZE || m_file.fail()) return false;

        uint8_t frameType = std::to_integer<uint8_t>(header[0]);
        isKeyFrame = (frameType == 0);
        uint32_t frameSize;
        std::memcpy(&frameSize, &header[1], sizeof(frameSize));
        frameData.resize(frameSize);
        m_file.read(reinterpret_cast<char *>(frameData.data()), frameSize);

        return !m_file.fail();
    }

    /**
     * @brief Closes the file
     */
    void close() {
        if (m_file.is_open()) m_file.close();
        m_isOpen = false;
    }

    /**
     * @brief Gets the original video width
     */
    int getOriginalWidth() const { return m_originalWidth; }

    /**
     * @brief Gets the original video height
     */
    int getOriginalHeight() const { return m_originalHeight; }

    /**
     * @brief Gets the original video frame rate
     */
    double getOriginalFPS() const { return m_originalFPS; }

    /**
     * @brief Gets the algorithm ID
     */
    uint16_t getAlgorithmId() const { return m_algorithmId; }

    /**
     * @brief Checks if the file is open
     */
    bool isOpen() const { return m_file.is_open(); }

  private:
    std::fstream m_file;
    bool m_isOpen;
    bool m_isWriteMode;
    int m_originalWidth;
    int m_originalHeight;
    double m_originalFPS;
    uint16_t m_algorithmId;
};

} // namespace utils
} // namespace vcompress