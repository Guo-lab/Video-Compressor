#include "utils/file_reader.hpp"
#include <iostream>

namespace vcompress {
namespace utils {

/// @brief Constructor
FileReader::FileReader() : m_isOpen(false), m_width(0), m_height(0), m_fps(0), m_frameCount(0), m_fourcc(0) {}

/// @brief Destructor
FileReader::~FileReader() { close(); }

/// @brief Cache video properties
void FileReader::updateVideoProperties() {
    m_width = static_cast<int>(m_videoCapture.get(cv::CAP_PROP_FRAME_WIDTH));
    m_height = static_cast<int>(m_videoCapture.get(cv::CAP_PROP_FRAME_HEIGHT));
    m_fps = m_videoCapture.get(cv::CAP_PROP_FPS);
    m_frameCount = static_cast<int>(m_videoCapture.get(cv::CAP_PROP_FRAME_COUNT));
    m_fourcc = static_cast<int>(m_videoCapture.get(cv::CAP_PROP_FOURCC));
    if (m_fps <= 0) m_fps = 30.0;
    if (m_frameCount <= 0) m_frameCount = 0;
}

/// @brief Open a video file for reading
bool FileReader::openFile(const std::string &filename) {
    if (m_isOpen) close();

    m_isOpen = m_videoCapture.open(filename);
    if (m_isOpen) {
        updateVideoProperties();
        std::cout << "Opened input video file: " << filename << std::endl;
        std::cout << "  Dimensions: " << m_width << "x" << m_height << std::endl;
        std::cout << "  FPS: " << m_fps << std::endl;
        std::cout << "  Duration: " << std::fixed << std::setprecision(2) << getDuration() << " seconds"
                  << std::endl;
        std::cout << "  Frame count: " << m_frameCount << std::endl;
    } else {
        std::cerr << "Failed to open input video file: " << filename << std::endl;
    }

    return m_isOpen;
}

/// @brief Check if the file is open
bool FileReader::isOpen() const { return m_isOpen; }

/// @brief Read the next frame from the video
bool FileReader::readNextFrame(cv::Mat &frame) {
    if (!m_isOpen) {
        return false;
    }
    return m_videoCapture.read(frame);
}

/// @brief Read the next frame and convert to the Frame format
bool FileReader::readNextFrame(algorithm::Frame &frame, int frameNumber) {
    cv::Mat cvFrame;
    if (!readNextFrame(cvFrame)) return false;

    frame.width = cvFrame.cols;
    frame.height = cvFrame.rows;
    frame.timestamp = frameNumber;
    frame.type = algorithm::KEY_FRAME;

    frame.data.resize(cvFrame.total() * cvFrame.elemSize());
    if (cvFrame.isContinuous()) {
        std::memcpy(frame.data.data(), cvFrame.data, frame.data.size());
    } else {
        size_t rowBytes = cvFrame.cols * cvFrame.elemSize();
        for (int i = 0; i < cvFrame.rows; ++i) {
            std::memcpy(frame.data.data() + i * rowBytes, cvFrame.ptr(i), rowBytes);
        }
    }

    return true;
}

/// @brief Get the width
int FileReader::getWidth() const { return m_width; }

/// @brief Get the height
int FileReader::getHeight() const { return m_height; }

/// @brief Get the frame rate
double FileReader::getFPS() const { return m_fps; }

/// @brief Get the frame count
int FileReader::getFrameCount() const { return m_frameCount; }

/// @brief Get the FourCC code
int FileReader::getFourCC() const { return m_fourcc; }

/// @brief Calculate duration based on frame count and frame rate
double FileReader::getDuration() const {
    if (m_fps > 0) {
        return static_cast<double>(m_frameCount) / m_fps;
    }
    return 0.0;
}

/// @brief Close the video file
void FileReader::close() {
    if (m_isOpen) {
        m_videoCapture.release();
        m_isOpen = false;
        m_width = 0;
        m_height = 0;
        m_fps = 0;
        m_frameCount = 0;
        m_fourcc = 0;
    }
}

} // namespace utils
} // namespace vcompress