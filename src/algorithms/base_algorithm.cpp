#include "algorithms/base_algorithm.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace vcompress {
namespace algorithm {

std::unordered_map<std::string, AlgorithmFactory::CreatorFunction> AlgorithmFactory::m_algorithm_creators;

/**
 * @brief Register a new algorithm with the factory
 *  Don't allow overwriting existing algorithms unless they're explicitly unregistered.
 *
 * @param name The name of the algorithm
 * @param creator The function to create an instance of the algorithm
 * @return true if the algorithm was successfully registered
 */
bool AlgorithmFactory::registerAlgorithm(const std::string &name, CreatorFunction creator) {
    if (m_algorithm_creators.find(name) != m_algorithm_creators.end()) {
        return false;
    }
    m_algorithm_creators[name] = creator;
    return true;
}

/**
 * @brief Unregister an algorithm from the factory
 *
 * @param name The name of the algorithm to unregister
 * @return true if the algorithm was successfully unregistered
 */
bool AlgorithmFactory::unregisterAlgorithm(const std::string &name) {
    return m_algorithm_creators.erase(name) > 0;
}

/**
 * @brief Get the list of available algorithms
 * @return std::vector<std::string> The list of available algorithms' names
 */
std::vector<std::string> AlgorithmFactory::getAvailableAlgorithms() {
    std::vector<std::string> algorithms(m_algorithm_creators.size());
    for (const auto &pair : m_algorithm_creators) {
        algorithms.push_back(pair.first);
    }
    return algorithms;
}

bool AlgorithmFactory::isAlgorithmAvailable(const std::string &name) {
    return m_algorithm_creators.find(name) != m_algorithm_creators.end();
}

/**
 * @brief Create an instance of an algorithm by name
 *
 * @param name The name of the algorithm to create
 * @return std::unique_ptr<BaseCompressionAlgorithm> The created algorithm instance
 */
std::unique_ptr<BaseCompressionAlgorithm> AlgorithmFactory::createAlgorithm(const std::string &name) {
    auto it = m_algorithm_creators.find(name);
    if (it != m_algorithm_creators.end()) {
        return it->second();
    }
    return nullptr;
}

} // namespace algorithm
} // namespace vcompress