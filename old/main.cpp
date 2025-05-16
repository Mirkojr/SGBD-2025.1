#include "bplustree.cpp"
#include <iostream>
#include <vector>

using namespace std;

int main() {
    BPlusTree<int> tree(4); // ordem 4
    tree.insert(10);
    tree.insert(20);
    tree.insert(5);
    tree.insert(6);
    tree.insert(12);
    tree.insert(30);
    tree.insert(7);
    tree.insert(17);

    tree.printTree();

    cout << "Busca 6: " << (tree.search(6) ? "Achou" : "Nao achou") << "\n";
    cout << "Busca 15: " << (tree.search(15) ? "Achou" : "Nao achou") << "\n";

    return 0;
}
