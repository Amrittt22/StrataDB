#ifndef STRATADB_SKIPLIST_H
#define STRATADB_SKIPLIST_H

#include <string>
#include <vector>
#include <random>

class SkipList {
private:
    static constexpr int MAX_LEVEL = 16;
    static constexpr float P = 0.5f;

    struct Node {
        std::string key;
        std::string value;
        std::vector<Node*> forward;

        Node(const std::string& k,
             const std::string& v,
             int level)
            : key(k), value(v), forward(level, nullptr) {}
    };

    Node* head;
    int currentLevel;

    std::mt19937 generator;
    std::uniform_real_distribution<float> distribution;

    int randomLevel();

public:
    SkipList();

    ~SkipList();

    // Prevent accidental shallow copies of owned nodes.
    SkipList(const SkipList&) = delete;
    SkipList& operator=(const SkipList&) = delete;

    void insert(const std::string& key,
                const std::string& value);

    bool search(const std::string& key,
                std::string& value) const;

    bool remove(const std::string& key);
};

#endif
