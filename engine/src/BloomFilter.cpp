#include "BloomFilter.h"

#include <cmath>
#include <functional>
#include <stdexcept>

BloomFilter::BloomFilter(
    std::size_t expectedItems,
    double falsePositiveRate
) {
    if (falsePositiveRate <= 0.0 ||
        falsePositiveRate >= 1.0) {
        throw std::invalid_argument(
            "False positive rate must be between 0 and 1"
        );
    }

    if (expectedItems == 0) {
        expectedItems = 1;
    }

    const double n = static_cast<double>(expectedItems);
    const double p = falsePositiveRate;
    const double calculatedBits =
        -n * std::log(p) /
        (std::log(2.0) * std::log(2.0));

    if (calculatedBits >
        static_cast<double>(bits.max_size())) {
        throw std::length_error("Bloom Filter is too large");
    }

    bitCount = static_cast<std::size_t>(std::ceil(calculatedBits));
    if (bitCount == 0) {
        bitCount = 1;
    }

    const double calculatedHashes =
        (static_cast<double>(bitCount) / n) * std::log(2.0);
    hashCount = static_cast<std::size_t>(std::round(calculatedHashes));
    if (hashCount == 0) {
        hashCount = 1;
    }

    bits.resize(bitCount, false);
}

std::size_t BloomFilter::hash1(const std::string& key) const {
    return std::hash<std::string>{}(key);
}

std::size_t BloomFilter::hash2(const std::string& key) const {
    std::size_t hash = 5381;

    for (unsigned char character : key) {
        hash = ((hash << 5) + hash) + character;
    }

    return hash | 1;
}

void BloomFilter::add(const std::string& key) {
    const std::size_t first = hash1(key);
    const std::size_t second = hash2(key);

    for (std::size_t i = 0; i < hashCount; i++) {
        bits[(first + i * second) % bitCount] = true;
    }
}

bool BloomFilter::mightContain(const std::string& key) const {
    const std::size_t first = hash1(key);
    const std::size_t second = hash2(key);

    for (std::size_t i = 0; i < hashCount; i++) {
        if (!bits[(first + i * second) % bitCount]) {
            return false;
        }
    }

    return true;
}

std::size_t BloomFilter::size() const {
    return bitCount;
}
