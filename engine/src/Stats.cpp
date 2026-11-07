#include "Stats.h"

void Stats::recordPut() {
    putCount++;
}

void Stats::recordGet(bool found) {
    getCount++;

    if (found) {
        successfulGets++;
    } else {
        failedGets++;
    }
}

void Stats::recordDelete() {
    deleteCount++;
}

std::uint64_t Stats::getPutCount() const {
    return putCount;
}

std::uint64_t Stats::getGetCount() const {
    return getCount;
}

std::uint64_t Stats::getDeleteCount() const {
    return deleteCount;
}

std::uint64_t Stats::getSuccessfulGets() const {
    return successfulGets;
}

std::uint64_t Stats::getFailedGets() const {
    return failedGets;
}

std::uint64_t Stats::getTotalOperations() const {
    return putCount + getCount + deleteCount;
}