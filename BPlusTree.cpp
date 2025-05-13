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
        int n; //number of elements in the node in this moment
    }

    Node* root; 
    int order; //order of the b+ tree
    BPlusTree(int order){
        this->order = order;
        root = nullptr;
    }
    ~BPlusTree() {
        deleteTree(root);
    }

    void insert(const T& key) {
        //check if the tree is empty and create a new root
        if (root == nullptr) {
            root = createEmptyNode(true);
            root->keys[0] = key;
            root->n = 1;
        } else {
            // find the leaf node where the key should be inserted
            Node* leafNode = findLeafNode(root, key);
            if (leafNode->n < this->order - 1) {  //check if the leaf node has space
                insertInLeaf(leafNode, key); 
            } 
            else {
                //handle splitting the leaf node when it's full
                Node* newLeafNode = createEmptyNode(true);
                
                // TODO: implement splitting logic
            }
        }       
    }

    private:

        Node* createEmptyNode(bool isLeaf) {
            Node* newNode = new Node();
            newNode->keys = new T[this->order];
            newNode->isLeaf = isLeaf;
            newNode->n = 0;
            newNode->next = nullptr;
            newNode->before = nullptr;
            return newNode;
        }

        void insertInLeaf(Node* leafNode, const T& key,  Node*) {
            leafNode -> n++;
            if (key < leafNode->keys[0]){
                leafNode->keys.insert(leafNode->keys.begin(), key);
            } else {
                for (size_t i = 1; i < this->order; ++i) {
                    if (key < leafNode->keys[i]) {
                        leafNode->keys.insert(leafNode->keys.begin() + i, key);
                        break;
                    }
                }
            }
        }

        Node* findLeafNode(Node* currentNode, const T& key) {
            if (currentNode->isLeaf) {
                return currentNode;
            }

            for (size_t i = 0; i < currentNode->keys.size(); ++i) {
                if (key < currentNode->keys[i]) {
                    return findLeafNode(currentNode->children[i], key);
                }
            }

            return findLeafNode(currentNode->children.back(), key);
        }
    

};
