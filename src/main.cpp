#include "algorithms/base_algorithm.hpp"
#include "algorithms/bilinear_downsample_algorithm.hpp"
#include "algorithms/cv_downsample_algorithm.hpp"
#include "core/decoder.hpp"
#include "core/encoder.hpp"
#include "utils/audio.hpp"
#include "utils/compressed_format.hpp"
#include "utils/file_reader.hpp"
#include "utils/file_writer.hpp"

// Configuration for the main program
struct MainConfig {
    std::string inputPath;
    std::string outputPath;
    std::string compressedDataPath = "data.vcomp";
    std::string tempVideoPath = "temp_processed_video.mp4";
    std::string tempAudioPath = "temp_audio.aac";
    std::string algorithmName = "CVDownsample";
    int quality = 20;
    int bitrate = 0;
    int keyFrameInterval = 30;
    bool keepAudio = true;
    bool keepTempFiles = false;
};

void printUsage(const char *programName) {
    std::cout << "Usage: " << programName << " <input_video> <output_video> [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -a, --algo      Compression algorithm (default: CVDownsample)" << std::endl;
    std::cout << "  -q, --quality   Quality level (1-100, default: 75)" << std::endl;
    std::cout << "  -l, --list      List available algorithms" << std::endl;
    std::cout << "  -h, --help      Show this help message" << std::endl;
    std::cout << "  --keep-temp     Keep temporary files after processing" << std::endl;
}

// Register the downsample algorithm
void registerAlgorithms() {
    using namespace vcompress::algorithm;
    AlgorithmFactory::registerAlgorithm("CVDownsample", []() -> std::unique_ptr<BaseCompressionAlgorithm> {
        return std::make_unique<CVDownsampleAlgorithm>();
    });
    AlgorithmFactory::registerAlgorithm("BilinearDownsample",
                                        []() -> std::unique_ptr<BaseCompressionAlgorithm> {
                                            return std::make_unique<BilinearDownsampleAlgorithm>();
                                        });
#ifdef USE_CUDA
    AlgorithmFactory::registerAlgorithm("CudaBilinearDownsample",
                                        []() -> std::unique_ptr<BaseCompressionAlgorithm> {
                                            return std::make_unique<CudaBilinearDownsampleAlgorithm>();
                                        });
#endif
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
            return true; }}
    };
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

    bool encodeMode = true;
    if (encodeMode) {
        vcompress::core::EncoderConfig encoderConfig(
            config.inputPath, config.outputPath, config.algorithmName, config.quality, config.bitrate,
            config.keyFrameInterval, false, config.keepAudio, config.keepTempFiles);
        vcompress::core::VideoEncoder encoder;
        if (!encoder.configure(encoderConfig)) {
            std::cerr << "Failed to configure encoder" << std::endl;
            return -1;
        }
        if (!encoder.encode()) {
            std::cerr << "Failed to encode video" << std::endl;
            return -1;
        }
    }
    encodeMode = false;
    if (!encodeMode) {
        vcompress::core::DecoderConfig decoderConfig(config.inputPath, config.outputPath,
                                                     config.algorithmName, config.quality, config.keepAudio,
                                                     config.keepTempFiles);
        vcompress::core::VideoDecoder decoder;
        if (!decoder.configure(decoderConfig)) {
            std::cerr << "Failed to configure decoder" << std::endl;
            return -1;
        }
        if (!decoder.decode()) {
            std::cerr << "Failed to decode video" << std::endl;
            return -1;
        }
    }
    return 0;
}