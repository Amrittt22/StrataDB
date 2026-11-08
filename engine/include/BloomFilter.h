#ifndef STRATADB_BLOOM_FILTER_H
#define STRATADB_BLOOM_FILTER_H

#include <cstddef>
#include <string>
#include <vector>

class BloomFilter {
private:
    std::vector<bool> bits;
    std::size_t bitCount;
    std::size_t hashCount;

    std::size_t hash1(const std::string& key) const;
    std::size_t hash2(const std::string& key) const;

public:
    explicit BloomFilter(
        std::size_t expectedItems,
        double falsePositiveRate = 0.01
    );

    void add(const std::string& key);
    bool mightContain(const std::string& key) const;
    std::size_t size() const;
};

#endif
