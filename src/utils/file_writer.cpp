#include "utils/file_writer.hpp"
#include <iostream>

namespace vcompress {
namespace utils {

/// @brief Constructor
FileWriter::FileWriter() : m_isOpen(false), m_width(0), m_height(0), m_fps(0), m_fourcc(0), m_quality(75) {}

/// @brief Destructor
FileWriter::~FileWriter() { close(); }

/// @brief Open a video file for writing
bool FileWriter::openFile(const std::string &filename, int width, int height, double fps, int fourcc,
                          int quality) {
    if (m_isOpen) close();

    m_width = width;
    m_height = height;
    m_fps = fps;
    m_fourcc = fourcc;
    m_quality = quality;

    m_isOpen = m_videoWriter.open(filename, fourcc, fps, cv::Size(width, height), true);
    if (m_isOpen) {
        setQuality(quality);
        std::cout << "Opened output video file: " << filename << std::endl;
        std::cout << "  Dimensions: " << m_width << "x" << m_height << std::endl;
        std::cout << "  FPS: " << m_fps << std::endl;
        std::cout << "  Quality: " << m_quality << std::endl;
    } else {
        std::cerr << "Failed to open output video file: " << filename << std::endl;
    }

    return m_isOpen;
}

/// @brief Check if the file is open
bool FileWriter::isOpen() const { return m_isOpen; }

/// @brief Write in the form of cv::Mat to the video
bool FileWriter::writeFrame(const cv::Mat &frame) {
    if (!m_isOpen) return false;

    if (frame.cols != m_width || frame.rows != m_height) {
        std::cerr << "Frame dimensions don't match output video dimensions" << std::endl;
        std::cerr << "Expected " << m_width << "x" << m_height << " but got " << frame.cols << "x"
                  << frame.rows << std::endl;
        return false;
    }

    m_videoWriter.write(frame);
    return true;
}

/// @brief Write in the form of Frame to the video
bool FileWriter::writeFrame(const algorithm::Frame &frame) {
    if (!m_isOpen) return false;

    if (frame.width != m_width || frame.height != m_height) {
        std::cerr << "Frame dimensions don't match output video dimensions" << std::endl;
        std::cerr << "Expected " << m_width << "x" << m_height << " but got " << frame.width << "x"
                  << frame.height << std::endl;
        return false;
    }

    cv::Mat mat(frame.height, frame.width, CV_8UC3);
    std::memcpy(mat.data, frame.data.data(), frame.data.size());

    m_videoWriter.write(mat);
    return true;
}

/// @brief Set the quality of the output video
bool FileWriter::setQuality(int quality) {
    if (!m_isOpen) return false;

    // Try to set the quality property on the VideoWriter
    // Note: This may not work on all platforms/codecs
    m_quality = std::max(0, std::min(100, quality));
    return m_videoWriter.set(cv::VIDEOWRITER_PROP_QUALITY, m_quality);
}

/// @brief Close the currently open file
void FileWriter::close() {
    if (m_isOpen) {
        m_videoWriter.release();
        m_isOpen = false;
        m_width = 0;
        m_height = 0;
        m_fps = 0;
        m_fourcc = 0;
    }
}

} // namespace utils
} // namespace vcompress