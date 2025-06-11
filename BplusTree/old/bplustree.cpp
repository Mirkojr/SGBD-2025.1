#include <iostream>
#include <vector>

using namespace std;

template <typename T>
class BPlusTree {
public:
    struct Node {
        bool isLeaf;
        T* keys;
        vector<Node*> children;
        Node* next;
        Node* before;
        int n;  // número de chaves
        Node(int order) {
            isLeaf = false;
            keys = new T[order];
            n = 0;
            next = nullptr;
            before = nullptr;
        }
    };

    Node* root;
    int order;

    BPlusTree(int order) {
        this->order = order;
        root = nullptr;
    }

    
    void insert(const T& key) {
        if (root == nullptr) {
            root = createEmptyNode(true);
            root->keys[0] = key;
            root->n = 1;
            return;
        }

        Node* leaf = findLeafNode(root, key);
        if (leaf->n < order - 1) {
            insertInLeaf(leaf, key);
        } else {
            insertAndSplitLeaf(leaf, key);
        }
    }

    bool search(const T& key) {
        if (!root) return false;

        Node* leaf = findLeafNode(root, key);
        for (int i = 0; i < leaf->n; i++) {
            if (leaf->keys[i] == key) {
                return true;
            }
        }
        return false;
    }

    void printTree() {
        if (!root) {
            cout << "Árvore vazia.\n";
            return;
        }
        vector<Node*> currentLevel, nextLevel;
        currentLevel.push_back(root);
        while (!currentLevel.empty()) {
            for (Node* node : currentLevel) {
                cout << "[";
                for (int i = 0; i < node->n; ++i) {
                    cout << node->keys[i];
                    if (i < node->n - 1) cout << ", ";
                }
                cout << "] ";
            }
            cout << "\n";
            nextLevel.clear();
            for (Node* node : currentLevel) {
                if (!node->isLeaf) {
                    for (Node* child : node->children) {
                        nextLevel.push_back(child);
                    }
                }
            }
            currentLevel = nextLevel;
        }
    }

private:
    Node* createEmptyNode(bool isLeaf) {
        Node* newNode = new Node(order);
        newNode->isLeaf = isLeaf;
        newNode->n = 0;
        newNode->next = nullptr;
        newNode->before = nullptr;
        return newNode;
    }

    Node* findLeafNode(Node* currentNode, const T& key) {
        if (currentNode->isLeaf) {
            return currentNode;
        }
        int i = 0;
        while (i < currentNode->n && key >= currentNode->keys[i]) {
            i++;
        }
        return findLeafNode(currentNode->children[i], key);
    }

    void insertInLeaf(Node* leaf, const T& key) {
        int i = leaf->n - 1;
        while (i >= 0 && leaf->keys[i] > key) {
            leaf->keys[i + 1] = leaf->keys[i];
            i--;
        }
        leaf->keys[i + 1] = key;
        leaf->n++;
    }

    void insertAndSplitLeaf(Node* leaf, const T& key) {
        T* tempKeys = new T[order];
        int i = 0, j = 0;

        while (i < leaf->n && leaf->keys[i] < key) {
            tempKeys[j++] = leaf->keys[i++];
        }
        tempKeys[j++] = key;
        while (i < leaf->n) {
            tempKeys[j++] = leaf->keys[i++];
        }

        int split = (order + 1) / 2;

        leaf->n = 0;
        for (i = 0; i < split; i++) {
            leaf->keys[i] = tempKeys[i];
            leaf->n++;
        }

        Node* newLeaf = createEmptyNode(true);
        for (i = split; i < order; i++) {
            newLeaf->keys[newLeaf->n++] = tempKeys[i];
        }

        // Atualiza ponteiros da lista ligada
        newLeaf->next = leaf->next;
        if (newLeaf->next) newLeaf->next->before = newLeaf;
        leaf->next = newLeaf;
        newLeaf->before = leaf;

        delete[] tempKeys;

        if (leaf == root) {
            Node* newRoot = createEmptyNode(false);
            newRoot->keys[0] = newLeaf->keys[0];
            newRoot->children.push_back(leaf);
            newRoot->children.push_back(newLeaf);
            newRoot->n = 1;
            root = newRoot;
        } else {
            insertInParent(leaf, newLeaf->keys[0], newLeaf);
        }
    }

    void insertInParent(Node* left, const T& key, Node* right) {
        if (left == root) {
            Node* newRoot = createEmptyNode(false);
            newRoot->keys[0] = key;
            newRoot->children.push_back(left);
            newRoot->children.push_back(right);
            newRoot->n = 1;
            root = newRoot;
            return;
        }

        Node* parent = findParent(root, left);

        int i = 0;
        while (i < parent->n && parent->keys[i] < key) i++;

        parent->keys[parent->n] = T();
        parent->children.push_back(nullptr);

        for (int j = parent->n; j > i; j--) {
            parent->keys[j] = parent->keys[j - 1];
            parent->children[j + 1] = parent->children[j];
        }
        parent->keys[i] = key;
        parent->children[i + 1] = right;
        parent->n++;

        if (parent->n == order) {
            splitInternalNode(parent);
        }
    }

    void splitInternalNode(Node* node) {
        int midIndex = order / 2;
        T midKey = node->keys[midIndex];

        Node* rightNode = createEmptyNode(false);
        rightNode->n = node->n - midIndex - 1;

        for (int i = 0; i < rightNode->n; i++) {
            rightNode->keys[i] = node->keys[midIndex + 1 + i];
        }
        for (int i = 0; i <= rightNode->n; i++) {
            rightNode->children.push_back(node->children[midIndex + 1 + i]);
        }

        node->n = midIndex;
        node->children.resize(midIndex + 1);

        if (node == root) {
            Node* newRoot = createEmptyNode(false);
            newRoot->keys[0] = midKey;
            newRoot->children.push_back(node);
            newRoot->children.push_back(rightNode);
            newRoot->n = 1;
            root = newRoot;
        } else {
            insertInParent(node, midKey, rightNode);
        }
    }

    Node* findParent(Node* current, Node* child) {
        if (current->isLeaf || current->children.size() == 0) return nullptr;

        for (Node* c : current->children) {
            if (c == child) return current;
        }

        for (Node* c : current->children) {
            Node* parent = findParent(c, child);
            if (parent) return parent;
        }
        return nullptr;
    }
};
