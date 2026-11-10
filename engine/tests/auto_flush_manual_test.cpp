
#include "Database.h"

#include <iostream>
#include <string>

int main() {
    Database db("./auto_flush_test_data", 5);

    if (!db.open()) {
        std::cerr << "Failed to open database\n";
        return 1;
    }

    std::cout << "Database opened successfully\n";

    for (int i = 1; i <= 5; i++) {
        std::string key = "key" + std::to_string(i);
        std::string value = "value" + std::to_string(i);

        if (!db.put(key, value)) {
            std::cerr << "Failed to insert " << key << '\n';
            return 1;
        }

        std::cout << "Inserted: " << key
                  << " | MemTable entries: " << db.memTableSize()
                  << " | SSTables: " << db.sstableCount()
                  << '\n';
    }

    std::cout << "\n--- Final Results ---\n";
    std::cout << "MemTable entries: " << db.memTableSize() << '\n';
    std::cout << "SSTable count: " << db.sstableCount() << '\n';
    std::cout << "Auto flush successful: "
              << (db.wasLastAutoFlushSuccessful() ? "Yes" : "No")
              << '\n';

    return 0;
}
