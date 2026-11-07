#ifndef STRATADB_STATS_H
#define STRATADB_STATS_H

#include <cstddef>
#include <cstdint>

class Stats {
private:
    std::uint64_t putCount = 0;
    std::uint64_t getCount = 0;
    std::uint64_t deleteCount = 0;

    std::uint64_t successfulGets = 0;
    std::uint64_t failedGets = 0;

public:
    void recordPut();
    void recordGet(bool found);
    void recordDelete();

    std::uint64_t getPutCount() const;
    std::uint64_t getGetCount() const;
    std::uint64_t getDeleteCount() const;

    std::uint64_t getSuccessfulGets() const;
    std::uint64_t getFailedGets() const;

    std::uint64_t getTotalOperations() const;
};

#endif