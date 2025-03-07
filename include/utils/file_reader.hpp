#pragma once

#include "algorithms/base_algorithm.hpp"
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>

namespace vcompress {
namespace utils {

/**
 * @brief Handles reading video files and extracting frames
 *
 * This class encapsulates video file reading operations using OpenCV.
 * It provides a consistent interface for accessing video frames.
 */
class FileReader {
  public:
    /**
     * @brief Constructs a new FileReader object
     */
    FileReader();

    /**
     * @brief Destructor ensures proper cleanup
     */
    ~FileReader();

    /**
     * @brief Open a video file for reading
     *
     * @param filename Path to the video file
     * @return true if file opened successfully, false otherwise
     */
    bool openFile(const std::string &filename);

    /**
     * @brief Check if a file is currently open
     *
     * @return true if a file is open, false otherwise
     */
    bool isOpen() const;

    /**
     * @brief Read the next frame from the video
     *
     * @param frame Reference to store the read frame
     * @return true if a frame was successfully read, false otherwise
     */
    bool readNextFrame(cv::Mat &frame);

    /**
     * @brief Read the next frame and convert to our Frame format
     *
     * @param frame Reference to store the read frame
     * @param frameNumber Current frame number (for timestamp)
     * @return true if a frame was successfully read, false otherwise
     */
    bool readNextFrame(algorithm::Frame &frame, int frameNumber);

    /**
     * @brief Get the width of the video
     *
     * @return Video width in pixels
     */
    int getWidth() const;

    /**
     * @brief Get the height of the video
     *
     * @return Video height in pixels
     */
    int getHeight() const;

    /**
     * @brief Get the frame rate of the video
     *
     * @return Frame rate in frames per second
     */
    double getFPS() const;

    /**
     * @brief Get the total number of frames in the video
     *
     * @return Total frame count
     */
    int getFrameCount() const;

    /**
     * @brief Get the fourcc code of the video
     *
     * @return Fourcc code as an integer
     */
    int getFourCC() const;

    /**
     * @brief Gets the duration of the video in seconds
     */
    double getDuration() const;

    /**
     * @brief Close the currently open file
     */
    void close();

  private:
    cv::VideoCapture m_videoCapture;
    bool m_isOpen;
    int m_width;
    int m_height;
    double m_fps;
    int m_frameCount;
    int m_fourcc;

    /**
     * @brief Updates the internal video properties
     *
     * Called after a file is opened to cache video metadata
     */
    void updateVideoProperties();
};

} // namespace utils
} // namespace vcompress