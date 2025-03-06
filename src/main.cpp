#include "algorithms/base_algorithm.hpp"
#include "algorithms/downsample_algorithm.hpp"
#include "utils/audio.hpp"

// Configuration for the main program
struct MainConfig {
    std::string inputPath;
    std::string outputPath;
    std::string tempVideoPath = "temp_processed_video.mp4";
    std::string tempAudioPath = "temp_audio.aac";
    std::string algorithmName = "Downsample";
    int quality = 20;
    int bitrate = 0;
    int keyFrameInterval = 30;
    bool keepTempFiles = false;
};

void printUsage(const char *programName) {
    std::cout << "Usage: " << programName << " <input_video> <output_video> [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -a, --algo      Compression algorithm (default: Downsample)" << std::endl;
    std::cout << "  -q, --quality   Quality level (1-100, default: 75)" << std::endl;
    std::cout << "  -l, --list      List available algorithms" << std::endl;
    std::cout << "  -h, --help      Show this help message" << std::endl;
    std::cout << "  --keep-temp     Keep temporary files after processing" << std::endl;
}

// Register the downsample algorithm
void registerAlgorithms() {
    using namespace vcompress::algorithm;
    AlgorithmFactory::registerAlgorithm("Downsample", []() -> std::unique_ptr<BaseCompressionAlgorithm> {
        return std::make_unique<DownsampleAlgorithm>();
    });
}

// List available algorithms
void listAvailableAlgorithms() {
    std::cout << "Available algorithms:" << std::endl;
    auto algorithms = vcompress::algorithm::AlgorithmFactory::getAvailableAlgorithms();
    for (const auto &algo : algorithms) {
        std::cout << "  " << algo << std::endl;
    }
}

auto helpHandler = [](int &, int, char **argv, MainConfig &) {
    printUsage(argv[0]);
    return false;
};

auto listHandler = [](int &, int, char **, MainConfig &) {
    listAvailableAlgorithms();
    return false;
};

auto algorithmHandler = [](int &i, int argc, char **argv, MainConfig &config) {
    if (i + 1 < argc) {
        config.algorithmName = argv[++i];
    } else {
        std::cerr << "Error: Missing argument for -a/--algorithm" << std::endl;
        return false;
    }
    return true;
};

auto qualityHandler = [](int &i, int argc, char **argv, MainConfig &config) {
    if (i + 1 < argc) {
        config.quality = std::clamp(std::atoi(argv[++i]), 1, 100);
    } else {
        std::cerr << "Error: Missing argument for -q/--quality" << std::endl;
        return false;
    }
    return true;
};

// clang-format off
std::unordered_map<std::string, std::function<bool(int &i, int argc, char **argv, MainConfig &config)>>
    argHandlers = {
        {"-h", helpHandler}, {"--help", helpHandler},
        {"-l", listHandler}, {"--list", listHandler},
        {"-a", algorithmHandler}, {"--algorithm", algorithmHandler},
        {"-q", qualityHandler}, {"--quality", qualityHandler},
        {"--keep-temp", [](int &, int, char **, MainConfig &config) {
            config.keepTempFiles = true;
            return true;
    }}};
// clang-format on

// Validate the configuration
bool validateConfig(const MainConfig &config) {
    if (!vcompress::algorithm::AlgorithmFactory::isAlgorithmAvailable(config.algorithmName)) {
        std::cerr << "Error: Algorithm '" << config.algorithmName << "' is not available." << std::endl;
        listAvailableAlgorithms();
        return false;
    }
    return true;
}

// Parse command line options into the config structure
bool parseCommandLineOptions(int argc, char **argv, MainConfig &config) {
    if (argc < 3) {
        printUsage(argv[0]);
        return false;
    }

    config.inputPath = argv[1];
    config.outputPath = argv[2];

    for (int i = 3; i < argc; i++) {
        std::string arg = argv[i];
        auto handler = argHandlers.find(arg);
        if (handler != argHandlers.end()) {
            if (!handler->second(i, argc, argv, config)) return false;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return false;
        }
    }
    return validateConfig(config);
}

// Create and initialize the algorithm
std::unique_ptr<vcompress::algorithm::BaseCompressionAlgorithm> createAlgorithm(const MainConfig &config) {
    auto algorithm = vcompress::algorithm::AlgorithmFactory::createAlgorithm(config.algorithmName);
    if (!algorithm) {
        std::cerr << "Error: Failed to create algorithm: " << config.algorithmName << std::endl;
        return nullptr;
    }

    vcompress::algorithm::CompressionConfig algoConfig(config.quality, config.bitrate,
                                                       config.keyFrameInterval);
    if (!algorithm->initialize(algoConfig)) {
        std::cerr << "Error: Failed to initialize algorithm: " << config.algorithmName << std::endl;
        return nullptr;
    }
    return algorithm;
}

// Process video frames with compression algorithm
bool processVideo(const std::string &inputVideo, const std::string &outputVideo,
                  vcompress::algorithm::BaseCompressionAlgorithm *algorithm) {
    cv::VideoCapture cap(inputVideo);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open input video: " << inputVideo << std::endl;
        return false;
    }

    int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    double fps = cap.get(cv::CAP_PROP_FPS);
    int fourcc = cv::VideoWriter::fourcc('X', '2', '6', '4');
    std::cout << "Input video: " << width << "x" << height << " @ " << fps << " fps" << std::endl;

    cv::VideoWriter writer;
    writer.open(outputVideo, fourcc, fps, cv::Size(width, height), true);
    if (!writer.isOpened()) {
        std::cerr << "Error: Could not create output video: " << outputVideo << std::endl;
        return false;
    }

    cv::Mat frame, processed_frame;
    int frameCount = 0;
    while (cap.read(frame)) {
        vcompress::algorithm::Frame input_frame(frame.cols, frame.rows);
        input_frame.timestamp = frameCount;
        input_frame.data.resize(frame.total() * frame.elemSize());
        std::memcpy(input_frame.data.data(), frame.data, input_frame.data.size());

        std::vector<uint8_t> compressed_data = algorithm->compressFrame(input_frame);

        vcompress::algorithm::Frame output_frame = algorithm->decompressFrame(compressed_data);
        processed_frame = cv::Mat(output_frame.height, output_frame.width, CV_8UC3);
        std::memcpy(processed_frame.data, output_frame.data.data(), output_frame.data.size());
        writer.write(processed_frame);

        frameCount++;
        if (frameCount % 100 == 0) std::cout << "Processed " << frameCount << " frames..." << std::endl;
    }

    cap.release();
    writer.release();

    std::cout << "Completed processing " << frameCount << " frames." << std::endl;
    std::cout << algorithm->getStats() << std::endl;
    return true;
}

// Run the full compression pipeline
bool runCompressionPipeline(vcompress::algorithm::BaseCompressionAlgorithm *algorithm,
                            const MainConfig &config) {
    // Step 1: Extract audio from original video
    if (!extractAudio(config.inputPath, config.tempAudioPath)) {
        std::cerr << "Failed to extract audio from input video" << std::endl;
        return false;
    }

    // Step 2: Process video frames with compression algorithm
    if (!processVideo(config.inputPath, config.tempVideoPath, algorithm)) {
        std::cerr << "Failed to process video frames" << std::endl;
        return false;
    }

    // Step 3: Combine processed video with original audio
    if (!combineVideoAudio(config.tempVideoPath, config.tempAudioPath, config.outputPath)) {
        std::cerr << "Failed to combine video and audio" << std::endl;
        return false;
    }

    if (!config.keepTempFiles) {
        std::remove(config.tempVideoPath.c_str());
        std::remove(config.tempAudioPath.c_str());
    }
    return true;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printUsage(argv[0]);
        return -1;
    }

    registerAlgorithms();

    MainConfig config;
    if (!parseCommandLineOptions(argc, argv, config)) {
        return -1;
    }
    auto algorithm = createAlgorithm(config);
    if (!algorithm) {
        return -1;
    }

    if (!runCompressionPipeline(algorithm.get(), config)) {
        return -1;
    }
    std::cout << "Video processing completed successfully!" << std::endl;
    std::cout << "Output saved to: " << config.outputPath << std::endl;
    return 0;
}