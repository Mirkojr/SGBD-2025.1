#ifndef BPLUSTREE_H
#define BPLUSTREE_H

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath> // Para ceil

// Declaração antecipada
class BPlusTree;

struct Node {
    int id;         // Número da linha no arquivo de índice (base 1 para o ID do nó em si). 0 se ainda não persistido ou inválido.
    bool isLeaf;
    std::vector<int> keys;
    int numKeys;    // Número atual de chaves no nó

    // Para nós internos
    std::vector<int> childNodeIds; // IDs (IDs dos nós) dos nós filhos

    // Para nós folha
    std::vector<int> dataPointers; // Ponteiros (números de linha reais em vinhos.csv, base 1, incluindo cabeçalho)
    int prevLeafId; // ID do nó folha anterior (0 se nenhum)
    int nextLeafId; // ID do nó folha seguinte (0 se nenhum)

    int order;      // Número máximo de filhos (m) para um nó interno. Máximo de chaves é m-1.
    // bool dirty; // Isso será gerenciado pela classe BPlusTree para o nó em buffer

    Node(int m, bool leaf, int nodeId = 0)
        : id(nodeId), isLeaf(leaf), numKeys(0), prevLeafId(0), nextLeafId(0), order(m){
        // Máximo de chaves é order-1. Reserva espaço, +1 para estouro temporário durante a divisão.
        keys.reserve(m); 
        if (isLeaf) {
            dataPointers.reserve(m); 
        } else {
            childNodeIds.reserve(m + 1); // Máximo de filhos da ordem
        }
    }

    bool isFull() const {
        return numKeys == order - 1;
    }

    // Para depuração (será chamado no nó em buffer)
    void printNodeDetails() const { // Renomeado para evitar conflito se BPlusTree tiver printNode
        std::cout << "Node ID: " << id << (isLeaf ? " (Folha)" : " (Interno)") << " Chaves: " << numKeys << "/" << order -1 << std::endl;
        std::cout << "  Chaves: ";
        for (int i = 0; i < numKeys; ++i) std::cout << keys[i] << " ";
        std::cout << std::endl;
        if (isLeaf) {
            std::cout << "  Ponteiros de Dados (linha em vinhos.csv): ";
            for (int i = 0; i < numKeys; ++i) std::cout << dataPointers[i] << " ";
            std::cout << std::endl;
            std::cout << "  Ant: " << prevLeafId << " Prox: " << nextLeafId << std::endl;
        } else {
            std::cout << "  IDs dos Filhos: ";
            for (size_t i = 0; i < childNodeIds.size(); ++i) std::cout << childNodeIds[i] << " ";
            std::cout << std::endl;
        }
    }
};

class BPlusTree {
public:
    BPlusTree(int order, const std::string& indexFileName, const std::string& dataFileName);
    ~BPlusTree();

    void insert(int key, int dataRecordId); // dataRecordId é o número real da linha em vinhos.csv
    std::vector<int> search(int key); // Retorna vetor de dataRecordIds (números de linha em vinhos.csv)
    void printTreeForDebug(); // Para depuração da árvore

private:
    int treeOrder;
    int rootNodeId;
    std::string indexFilePath;
    std::string dataFilePath; // vinhos.csv
    int nextNodeIdCounter;    // Rastreia o próximo ID disponível para um novo nó

    // Buffer para um nó de índice
    Node* currentIndexNodeInRam;
    int currentIndexNodeInRamId;
    bool currentIndexNodeDirty;

    // Buffer para uma página de dados (registro de vinhos.csv)
    std::string currentDataRecordInRam; // Armazena o conteúdo da linha
    int currentDataRecordInRamId;   // Armazena o número da linha (base 1) de vinhos.csv

    // Gerenciamento de buffer de nó
    Node* accessNode(int nodeId); // Garante que o nó esteja em currentIndexNodeInRam, retorna-o. Lida com carga/salvamento do anterior.
    void markCurrentNodeDirty();
    Node* createNewBufferedNode(bool isLeaf); // Cria um novo nó, coloca-o no buffer, atribui ID.

    // Gerenciamento de buffer de dados
    std::string accessDataRecord(int recordLineNumber); // Garante que o registro de dados esteja em currentDataRecordInRam

    // Operações centrais da Árvore B+ (usarão accessNode, markCurrentNodeDirty, createNewBufferedNode)
    int findLeafNodeIdToInsert(int key, std::vector<int>& pathNodeIds);
    void insertIntoLeafNonFull(int leafNodeId, int key, int dataRecordId);
    void splitAndInsertLeaf(int leafNodeId, int key, int dataRecordId, std::vector<int>& pathNodeIds);
    void insertIntoParent(int oldChildNodeId, int keyToPushUp, int newChildNodeId, std::vector<int>& pathNodeIds);
    // splitInternalNode faz parte de insertIntoParent se o pai estiver cheio
    void createNewRootAndUpdate(int oldLeftChildId, int key, int oldRightChildId);

    // E/S de Arquivo e Análise (permanecem basicamente os mesmos, mas interagem com a lógica do buffer)
    Node* loadNodeFromFile(int nodeId); // Lógica real de leitura de arquivo
    void saveNodeToFile(Node* node);   // Lógica real de escrita de arquivo
    void initializeIndexFile(); // Renomeado de initializeIndexFileIfEmpty para clareza
    std::string readLineFromFile(const std::string& filePath, int lineNumber); 
    void writeLineToFile(const std::string& filePath, int lineNumber, const std::string& content);
    void appendLineToFile(const std::string& filePath, const std::string& content); // Pode não ser necessário se writeLineToFile preencher
    int countLinesInFile(const std::string& filePath); // Renomeado

    Node* parseNodeString(const std::string& line, int nodeIdFromFile);
    std::string formatNodeString(Node* node);

    // Auxiliares de depuração
    void printNodeRecursive(int nodeId, int level);

    static const int HEADER_LINES = 2; // Para ROOT_ID e NEXT_NODE_ID no arquivo de índice
};

#endif // BPLUSTREE_H

