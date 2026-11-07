#include "SkipList.h"
#include <cstddef>
#include <iostream>

SkipList::SkipList()
    : currentLevel(1),
      generator(std::random_device{}()),
      distribution(0.0f, 1.0f) {

    head = new Node("", "", MAX_LEVEL);
}

SkipList::~SkipList() {
    Node* current = head->forward[0];

    while (current != nullptr) {
        Node* next = current->forward[0];
        delete current;
        current = next;
    }

    delete head;
}

int SkipList::randomLevel() {
    int level = 1;

    while (distribution(generator) < P &&
           level < MAX_LEVEL) {
        level++;
    }

    return level;
}
void SkipList::insert(const std::string& key,
                      const std::string& value) {

    std::vector<Node*> update(MAX_LEVEL, nullptr);

    Node* current = head;

    // Find the insertion position.
    for (int i = currentLevel - 1; i >= 0; i--) {

        while (current->forward[i] != nullptr &&
               current->forward[i]->key < key) {

            current = current->forward[i];
        }

        update[i] = current;
    }

    // Check whether the key already exists.
    current = current->forward[0];

    if (current != nullptr && current->key == key) {
        current->value = value;
        return;
    }

    // Generate a random level for the new node.
    int newLevel = randomLevel();

    if (newLevel > currentLevel) {
        for (int i = currentLevel; i < newLevel; i++) {
            update[i] = head;
        }

        currentLevel = newLevel;
    }

    Node* newNode = new Node(key, value, newLevel);

    // Update forward pointers.
    for (int i = 0; i < newLevel; i++) {
        newNode->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = newNode;
    }
}

bool SkipList::search(const std::string& key,
                      std::string& value) const {

    Node* current = head;

    // Start from the highest level.
    for (int i = currentLevel - 1; i >= 0; i--) {

        while (current->forward[i] != nullptr &&
               current->forward[i]->key < key) {

            current = current->forward[i];
        }
    }

    // Move to the possible target node.
    current = current->forward[0];

    if (current != nullptr && current->key == key) {
        value = current->value;
        return true;
    }

    return false;
}
bool SkipList::remove(const std::string& key) {
    Node* current = head;
    std::vector<Node*> update(MAX_LEVEL, nullptr);

    // Find the node and track predecessors at every level
    for (int i = currentLevel - 1; i >= 0; i--) {
        while (current->forward[i] != nullptr &&
               current->forward[i]->key < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    current = current->forward[0];

    // Key doesn't exist
    if (current == nullptr || current->key != key) {
        return false;
    }

    // Unlink the node from every level it occupies
    for (int i = 0; i < static_cast<int>(current->forward.size()); i++) {
        if (update[i]->forward[i] == current) {
            update[i]->forward[i] = current->forward[i];
        }
    }

    delete current;

    // Reduce the current level if necessary
    while (currentLevel > 1 &&
           head->forward[currentLevel - 1] == nullptr) {
        currentLevel--;
    }

    return true;
}