#include "Database.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

struct LatencySummary {
    double totalMilliseconds;
    double operationsPerSecond;
    double averageMicroseconds;
    double p50Microseconds;
    double p95Microseconds;
    double p99Microseconds;
};

LatencySummary summarize(std::vector<double> samples) {
    if (samples.empty()) {
        return {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    }

    std::sort(samples.begin(), samples.end());
    const double totalMicroseconds =
        std::accumulate(samples.begin(), samples.end(), 0.0);
    const auto percentile = [&](double fraction) {
        const std::size_t index = static_cast<std::size_t>(
            fraction * static_cast<double>(samples.size() - 1)
        );
        return samples[index];
    };

    return {
        totalMicroseconds / 1000.0,
        totalMicroseconds == 0.0
            ? 0.0
            : 1000000.0 * samples.size() / totalMicroseconds,
        totalMicroseconds / samples.size(),
        percentile(0.50),
        percentile(0.95),
        percentile(0.99)
    };
}

double elapsedMicroseconds(const Clock::time_point start) {
    return std::chrono::duration<double, std::micro>(
        Clock::now() - start
    ).count();
}

std::string keyFor(
    std::size_t index,
    std::size_t keyWidth
) {
    std::ostringstream key;
    key << "key-" << std::setw(static_cast<int>(keyWidth))
        << std::setfill('0') << index;
    return key.str();
}

std::string valueFor(std::size_t index) {
    return "value-" + std::to_string(index);
}

void writeHeader(std::ofstream& output) {
    output
        << "operation,dataset_size,total_operations,total_time_ms,"
        << "throughput_ops_sec,avg_latency_us,p50_latency_us,"
        << "p95_latency_us,p99_latency_us,returned_records\n";
}

void writeResult(
    std::ofstream& output,
    const std::string& operation,
    std::size_t datasetSize,
    std::size_t operationCount,
    const LatencySummary& summary,
    std::size_t returnedRecords = 0
) {
    output << operation << ','
           << datasetSize << ','
           << operationCount << ','
           << std::fixed << std::setprecision(3)
           << summary.totalMilliseconds << ','
           << summary.operationsPerSecond << ','
           << summary.averageMicroseconds << ','
           << summary.p50Microseconds << ','
           << summary.p95Microseconds << ','
           << summary.p99Microseconds << ','
           << returnedRecords << '\n';
}

bool prepareDatabase(
    const fs::path& directory,
    std::size_t recordCount,
    std::size_t batchSize,
    std::size_t keyWidth
) {
    Database database(directory.string(), 0);

    if (!database.open()) {
        return false;
    }

    for (std::size_t offset = 0; offset < recordCount; offset += batchSize) {
        const std::size_t end = std::min(recordCount, offset + batchSize);

        for (std::size_t index = offset; index < end; ++index) {
            if (!database.put(keyFor(index, keyWidth), valueFor(index))) {
                return false;
            }
        }

        if (!database.flushMemTable()) {
            return false;
        }
    }

    return true;
}

int main(int argc, char* argv[]) {
    const std::size_t recordCount = argc >= 2
        ? std::stoull(argv[1])
        : 1000;
    const fs::path outputPath = argc >= 3
        ? fs::path(argv[2])
        : fs::path("benchmarks") / "results" / "stratadb_benchmark.csv";
    const fs::path root =
        fs::temp_directory_path() / "stratadb_benchmark";

    if (recordCount == 0) {
        std::cerr << "Dataset size must be positive\n";
        return 1;
    }

    const std::size_t keyWidth =
        std::to_string(recordCount - 1).size();

    std::error_code error;
    fs::remove_all(root, error);
    if (!outputPath.parent_path().empty()) {
        fs::create_directories(outputPath.parent_path(), error);
    }

    std::ofstream output(outputPath);
    if (!output.is_open()) {
        std::cerr << "Unable to open benchmark output: "
                  << outputPath << '\n';
        return 1;
    }

    writeHeader(output);

    const fs::path putDirectory = root / "put";
    {
        Database database(putDirectory.string(), recordCount + 1);
        if (!database.open()) {
            std::cerr << "Unable to open PUT benchmark database\n";
            return 1;
        }

        std::vector<double> samples;
        samples.reserve(recordCount);

        for (std::size_t index = 0; index < recordCount; ++index) {
            const auto start = Clock::now();
            if (!database.put(keyFor(index, keyWidth), valueFor(index))) {
                std::cerr << "PUT benchmark operation failed\n";
                return 1;
            }
            samples.push_back(elapsedMicroseconds(start));
        }

        writeResult(
            output,
            "put_memtable",
            recordCount,
            recordCount,
            summarize(std::move(samples))
        );
    }

    const fs::path readDirectory = root / "read";
    if (!prepareDatabase(readDirectory, recordCount, recordCount, keyWidth)) {
        std::cerr << "Unable to prepare GET benchmark database\n";
        return 1;
    }

    {
        Database database(readDirectory.string(), 0);
        if (!database.open()) {
            std::cerr << "Unable to open GET benchmark database\n";
            return 1;
        }

        std::vector<double> samples;
        samples.reserve(recordCount);

        for (std::size_t index = 0; index < recordCount; ++index) {
            std::string value;
            const auto start = Clock::now();
            if (!database.get(keyFor(index, keyWidth), value) ||
                value != valueFor(index)) {
                std::cerr << "GET benchmark verification failed\n";
                return 1;
            }
            samples.push_back(elapsedMicroseconds(start));
        }

        writeResult(
            output,
            "get_sstable",
            recordCount,
            recordCount,
            summarize(std::move(samples))
        );
    }

    const fs::path scanDirectory = root / "scan";
    if (!prepareDatabase(scanDirectory, recordCount, recordCount, keyWidth)) {
        std::cerr << "Unable to prepare scan benchmark database\n";
        return 1;
    }

    {
        Database database(scanDirectory.string(), 0);
        if (!database.open()) {
            std::cerr << "Unable to open scan benchmark database\n";
            return 1;
        }

        std::vector<double> samples;
        samples.reserve(3);
        std::size_t returnedRecords = 0;

        for (int repeat = 0; repeat < 3; ++repeat) {
            const auto start = Clock::now();
            const auto rows = database.scan(
                keyFor(0, keyWidth),
                keyFor(recordCount - 1, keyWidth)
            );
            samples.push_back(elapsedMicroseconds(start));
            returnedRecords = rows.size();

            if (returnedRecords != recordCount) {
                std::cerr << "Range scan benchmark verification failed\n";
                return 1;
            }

            for (std::size_t index = 0; index < rows.size(); ++index) {
                if (rows[index].first != keyFor(index, keyWidth) ||
                    rows[index].second != valueFor(index)) {
                    std::cerr
                        << "Range scan benchmark value verification failed\n";
                    return 1;
                }
            }
        }

        const std::size_t operationCount = samples.size();
        writeResult(
            output,
            "range_scan",
            recordCount,
            operationCount,
            summarize(std::move(samples)),
            returnedRecords
        );
    }

    const fs::path compactionDirectory = root / "compaction";
    if (!prepareDatabase(compactionDirectory, recordCount, std::max<std::size_t>(
            1,
            recordCount / 4
        ), keyWidth)) {
        std::cerr << "Unable to prepare compaction benchmark database\n";
        return 1;
    }

    {
        Database database(compactionDirectory.string(), 0);
        if (!database.open()) {
            std::cerr << "Unable to open compaction benchmark database\n";
            return 1;
        }

        const auto start = Clock::now();
        if (!database.compact()) {
            std::cerr << "Compaction benchmark operation failed\n";
            return 1;
        }

        const double duration = elapsedMicroseconds(start);
        const auto compactedRows = database.scan(
            keyFor(0, keyWidth),
            keyFor(recordCount - 1, keyWidth)
        );

        if (compactedRows.size() != recordCount) {
            std::cerr
                << "Compaction range scan returned "
                << compactedRows.size()
                << " records; expected "
                << recordCount << '\n';
            return 1;
        }

        for (std::size_t index = 0; index < compactedRows.size(); ++index) {
            if (compactedRows[index].first != keyFor(index, keyWidth) ||
                compactedRows[index].second != valueFor(index)) {
                std::cerr
                    << "Compaction range scan value verification failed\n";
                return 1;
            }
        }

        writeResult(
            output,
            "compaction",
            recordCount,
            1,
            summarize({duration}),
            compactedRows.size()
        );
    }

    output.close();
    fs::remove_all(root, error);

    std::cout << "Benchmark results written to "
              << outputPath << '\n';
    return 0;
}
