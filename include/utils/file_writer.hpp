#pragma once

#include "algorithms/base_algorithm.hpp"
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>

namespace vcompress {
namespace utils {

/**
 * @brief Handles writing video frames to a file
 *
 * This class encapsulates video file writing operations using OpenCV.
 * It provides a consistent interface for writing video frames.
 */
class FileWriter {
  public:
    /**
     * @brief Default constructor
     */
    FileWriter();

    /**
     * @brief Destructor
     */
    ~FileWriter();

    /**
     * @brief Open a video file for writing
     *
     * @param filename Path to the video file to create
     * @param width Width of the video in pixels
     * @param height Height of the video in pixels
     * @param fps Frame rate in frames per second
     * @param fourcc FourCC code for the video codec
     * @param quality Video quality (0-100, where higher is better quality)
     * @return true if file opened successfully, false otherwise
     */
    bool openFile(const std::string &filename, int width, int height, double fps,
                  int fourcc = cv::VideoWriter::fourcc('H', '2', '6', '4'), int quality = 75);

    /**
     * @brief Check if a file is currently open for writing
     *
     * @return true if a file is open, false otherwise
     */
    bool isOpen() const;

    /**
     * @brief Write a frame to the video
     *
     * @param frame OpenCV frame to write
     * @return true if frame was successfully written, false otherwise
     */
    bool writeFrame(const cv::Mat &frame);

    /**
     * @brief Write a frame to the video
     *
     * @param frame Frame in our format to write
     * @return true if frame was successfully written, false otherwise
     */
    bool writeFrame(const algorithm::Frame &frame);

    /**
     * @brief Set the quality of the output video
     *
     * @param quality Quality value (0-100, where higher is better quality)
     * @return true if quality was set successfully, false otherwise
     */
    bool setQuality(int quality);

    /**
     * @brief Close the currently open file
     */
    void close();

  private:
    cv::VideoWriter m_videoWriter;
    bool m_isOpen;
    int m_width;
    int m_height;
    double m_fps;
    int m_fourcc;
    int m_quality;
};

} // namespace utils
} // namespace vcompress