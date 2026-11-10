
#include "Database.h"

#include <iostream>
#include <string>

int main() {
    Database db("./auto_flush_test_data", 5);

    if (!db.open()) {
        std::cerr << "Failed to reopen database\n";
        return 1;
    }

    std::cout << "Database reopened successfully\n";

    bool allPassed = true;

    for (int i = 1; i <= 5; i++) {
        std::string key = "key" + std::to_string(i);
        std::string expected = "value" + std::to_string(i);
        std::string actual;

        if (!db.get(key, actual)) {
            std::cout << "[FAIL] " << key << " not found\n";
            allPassed = false;
            continue;
        }

        if (actual != expected) {
            std::cout << "[FAIL] " << key
                      << " expected: " << expected
                      << ", got: " << actual << '\n';
            allPassed = false;
            continue;
        }

        std::cout << "[PASS] " << key << " = " << actual << '\n';
    }

    std::cout << "\n--- Recovery Results ---\n";
    std::cout << "MemTable entries: " << db.memTableSize() << '\n';
    std::cout << "SSTable count: " << db.sstableCount() << '\n';
    std::cout << "Recovery test: "
              << (allPassed ? "PASSED" : "FAILED") << '\n';

    return allPassed ? 0 : 1;
}
