#include "bplustree.h"
#include <filesystem> // Para filesystem::file_size
#include <limits>
#include <cmath> // Para ceil

#define HEADER_LINES 2

using namespace std;
/**
 * Construtor da classe BPlusTree.
 * Inicializa a árvore B+, define sua ordem, os nomes dos arquivos de índice e
 * dados, e carrega o estado da árvore (ID da raiz e próximo ID de nó
 * disponível) se o arquivo de índice já existir order A ordem da árvore B+
 * (número máximo de filhos para nós internos). indexFileName O nome do arquivo
 * que armazenará o índice da árvore B+ dataFileName O nome do arquivo CSV que
 * contém os dados a serem indexados (vinhos.csv)
 */
BPlusTree::BPlusTree(int order, const string &indexFileName,
                     const string &dataFileName)
    : treeOrder(order), rootNodeId(0), indexFilePath(indexFileName),
      dataFilePath(dataFileName),
      nextNodeIdCounter(
          1), // Contador para o ID do próximo nó a ser criado, começa em 1.
      currentIndexNodeInRam(
          nullptr), // Ponteiro para o nó de índice atualmente em buffer (RAM).
      currentIndexNodeInRamId(0), // ID do nó de índice atualmente em buffer.
      currentIndexNodeDirty(false), // Flag que indica se o nó em buffer foi
                                    // modificado e precisa ser salvo.
      currentDataRecordInRam(""),   // String para armazenar o registro de dados
                                    // atualmente em buffer.
      currentDataRecordInRamId(
          0) { // ID (número da linha) do registro de dados em buffer.
  initializeIndexFile(); // Garante que o arquivo de índice exista e tenha o
                         // cabeçalho básico.

  // Tenta ler o ID da raiz do arquivo de índice.
  string rootLine = readLineFromFile(indexFilePath, 1);
  if (!rootLine.empty() &&
      rootLine.rfind("ROOT_ID:", 0) == 0) { // rfind para verificar o prefixo
    try {
      rootNodeId = stoi(rootLine.substr(8)); // Extrai o ID após "ROOT_ID:"
    } catch (const exception &e) {
      cerr << "Erro ao analisar ROOT_ID: " << e.what() << endl;
      rootNodeId = 0; // Fallback: considera que não há raiz se houver erro.
    }
  } else {
    rootNodeId = 0; // Se não houver linha ROOT_ID ou o arquivo acabou de ser
                    // inicializado.
  }

  // Tenta ler o contador do próximo ID de nó do arquivo de índice.
  string nextIdLine = readLineFromFile(indexFilePath, 2);
  if (!nextIdLine.empty() && nextIdLine.rfind("NEXT_NODE_ID:", 0) == 0) {
    try {
      nextNodeIdCounter = stoi(nextIdLine.substr(13)); // Extrai o ID após "NEXT_NODE_ID:"
      cout << "nextNodeIdCounter: " << nextNodeIdCounter << endl;
    } catch (const exception &e) {
      cerr << "Erro ao analisar NEXT_NODE_ID: " << e.what() << endl;
      nextNodeIdCounter = 1; // Fallback: começa em 1 se houver erro.
    }
  } else {
    nextNodeIdCounter = 1; // Se não houver linha NEXT_NODE_ID ou o arquivo
                           // acabou de ser inicializado.
  }
}

/**
 * Destrutor da classe BPlusTree.
 * Garante que o nó de índice atualmente em buffer (se existir e estiver
 * modificado) seja salvo em disco. Atualiza as informações de cabeçalho (ID da
 * raiz e próximo ID de nó) no arquivo de índice.
 */
BPlusTree::~BPlusTree() {
  // Salva o nó de índice em RAM se estiver sujo (modificado).
  if (currentIndexNodeInRam != nullptr && currentIndexNodeDirty) {
    saveNodeToFile(currentIndexNodeInRam);
  }
  delete currentIndexNodeInRam; // Libera a memória do nó em buffer.
  currentIndexNodeInRam = nullptr;

  // Lê todas as linhas do arquivo de índice para atualizar o cabeçalho.
  vector<string> lines;
  ifstream inFile(indexFilePath);
  string currentLine;
  if (inFile.is_open()) {
    while (getline(inFile, currentLine)) {
      lines.push_back(currentLine);
    }
    inFile.close();
  }

  // Prepara as strings de cabeçalho atualizadas.
  string rootStr = "ROOT_ID:" + to_string(rootNodeId);
  string nextIdStr = "NEXT_NODE_ID:" + to_string(nextNodeIdCounter);

  // Atualiza ou adiciona as linhas de cabeçalho.
  if (lines.size() >= 2) {
    lines[0] = rootStr;
    lines[1] = nextIdStr;
  } else if (lines.size() == 1) {
    lines[0] = rootStr;
    lines.push_back(nextIdStr);
  } else {
    lines.push_back(rootStr);
    lines.push_back(nextIdStr);
  }

  // Escreve todas as linhas de volta no arquivo de índice, sobrescrevendo-o.
  ofstream outFile(indexFilePath, ios::trunc);
  if (outFile.is_open()) {
    for (const auto &l : lines) {
      outFile << l << endl;
    }
    outFile.close();
  } else {
    cerr << "Erro: Não foi possível abrir o arquivo de índice "
              << indexFilePath << " para salvar root/next ID no destrutor."
              << endl;
  }
}

/**
 * Inicializa o arquivo de índice se ele não existir ou estiver vazio.
 * Cria o arquivo com as linhas de cabeçalho para ROOT_ID (inicializado como 0)
 * e NEXT_NODE_ID (inicializado como 1).
 */
void BPlusTree::initializeIndexFile() {
  ifstream fileCheck(indexFilePath);
  bool fileExists = fileCheck.good();
  long fileSize = 0;
  if (fileExists) {
    fileCheck.seekg(0, ios::end);
    fileSize = fileCheck.tellg();
    fileCheck.close(); // Fecha o arquivo após obter o tamanho.
  } else {
    fileCheck
        .close(); // Garante que o arquivo seja fechado mesmo se não existir.
  }

  if (!fileExists || fileSize == 0) {
    ofstream file(indexFilePath,
                       ios::trunc); // Cria/sobrescreve o arquivo.
    if (file.is_open()) {
      file << "ROOT_ID:0\n";      // Raiz inicialmente 0 (árvore vazia).
      file << "NEXT_NODE_ID:1\n"; // Próximo ID de nó começa em 1.
      file.close();
      rootNodeId = 0; // Atualiza o estado interno.
      nextNodeIdCounter = 1;
    } else {
      cerr
          << "Erro: Não foi possível criar ou inicializar o arquivo de índice: "
          << indexFilePath << endl;
    }
  }
}

/**
 * Acessa um nó da árvore B+ pelo seu ID, gerenciando o buffer de nó de índice.
 * Se o nó solicitado já estiver no buffer, retorna-o diretamente.
 * Caso contrário, se houver um nó diferente no buffer e ele estiver sujo
 * (modificado), salva-o em disco primeiro. Em seguida, carrega o nó solicitado
 * do disco para o buffer e o retorna. ai nodeId O ID do nó a ser acessado.
 * Ponteiro para o nó no buffer de índice, ou nullptr se o nodeId for 0 ou o nó
 * não puder ser carregado.
 */
Node *BPlusTree::accessNode(int nodeId) {
  if (nodeId == 0)
    return nullptr; // ID 0 é inválido ou representa nulo.

  // Se o nó desejado já está no buffer, retorna-o.
  if (currentIndexNodeInRam != nullptr && currentIndexNodeInRamId == nodeId) {
    return currentIndexNodeInRam;
  }

  // Se há um nó no buffer e ele está sujo, salva-o antes de carregar outro.
  if (currentIndexNodeInRam != nullptr && currentIndexNodeDirty) {
    saveNodeToFile(currentIndexNodeInRam);
  }
  delete currentIndexNodeInRam; // Libera o nó antigo do buffer.
  currentIndexNodeInRam = nullptr;

  // Carrega o novo nó do arquivo para o buffer.
  currentIndexNodeInRam = loadNodeFromFile(nodeId);
  if (currentIndexNodeInRam != nullptr) {
    currentIndexNodeInRamId = nodeId; // Atualiza o ID do nó em buffer.
    currentIndexNodeDirty = false;    // O nó recém-carregado não está sujo.
  } else {
    currentIndexNodeInRamId = 0; // Falha ao carregar, reseta o ID do buffer.
  }
  return currentIndexNodeInRam;
}

/**
 * Marca o nó de índice atualmente em buffer como "sujo" (modificado).
 * Isso indica que o nó precisará ser salvo de volta no arquivo de índice antes
 * de ser substituído no buffer.
 */
void BPlusTree::markCurrentNodeDirty() {
  if (currentIndexNodeInRam != nullptr) {
    currentIndexNodeDirty = true;
  }
}

/**
 * Cria um novo nó (folha ou interno) e o coloca no buffer de índice.
 * Se houver um nó sujo no buffer, ele é salvo primeiro.
 * O novo nó recebe um ID do `nextNodeIdCounter`, é colocado no buffer, marcado
 * como sujo e salvo imediatamente no arquivo de índice para garantir sua
 * persistência inicial. isLeaf True se o novo nó deve ser uma folha, False caso
 * contrário. retorna Ponteiro para o novo nó criado, agora no buffer de índice.
 */
Node *BPlusTree::createNewBufferedNode(bool isLeaf) {
  // Salva o nó atual em RAM se estiver sujo.
  if (currentIndexNodeInRam != nullptr && currentIndexNodeDirty) {
    saveNodeToFile(currentIndexNodeInRam);
  }
  delete currentIndexNodeInRam; // Libera o nó antigo do buffer.

  int newNodeId =
      nextNodeIdCounter++; // Obtém e incrementa o ID para o novo nó.
  currentIndexNodeInRam =
      new Node(treeOrder, isLeaf, newNodeId); // Cria o novo nó.
  currentIndexNodeInRamId = newNodeId;        // Atualiza o ID do nó em buffer.
  currentIndexNodeDirty =
      true; // O novo nó é considerado sujo pois precisa ser escrito.
  saveNodeToFile(
      currentIndexNodeInRam); // Salva o novo nó no arquivo imediatamente.
  return currentIndexNodeInRam;
}

/**
 * Acessa um registro de dados (uma linha do arquivo vinhos.csv) pelo seu número
 * de linha, gerenciando o buffer de dados. Se o registro solicitado já estiver
 * no buffer de dados, retorna-o. Caso contrário, carrega o registro do arquivo
 * de dados para o buffer e o retorna. recordLineNumber O número da linha
 * (1-based) do registro a ser acessado no arquivo de dados. retorna String
 * contendo o registro de dados, ou string vazia se o recordLineNumber for 0 ou
 * o registro não puder ser lido.
 */
string BPlusTree::accessDataRecord(int recordLineNumber) {
  if (recordLineNumber == 0)
    return ""; // Número de linha 0 é inválido.

  // Se o registro desejado já está no buffer, retorna-o.
  if (currentDataRecordInRamId == recordLineNumber &&
      !currentDataRecordInRam.empty()) {
    return currentDataRecordInRam;
  }

  // Carrega o registro do arquivo de dados para o buffer.
  currentDataRecordInRam = readLineFromFile(dataFilePath, recordLineNumber);
  if (!currentDataRecordInRam.empty()) {
    currentDataRecordInRamId =
        recordLineNumber; // Atualiza o ID do registro em buffer.
  } else {
    currentDataRecordInRamId = 0; // Falha ao carregar, reseta o ID do buffer.
  }
  return currentDataRecordInRam;
}

/**
 * Lê uma linha específica de um arquivo.
 * filePath O caminho para o arquivo.
 * lineNumber O número da linha a ser lida (1-based).
 * retorna String contendo a linha lida, ou string vazia se o arquivo não puder
 * ser aberto ou a linha não existir.
 */
string BPlusTree::readLineFromFile(const string &filePath,
                                        int lineNumber) {
  ifstream file(filePath);
  string lineContent;
  if (!file.is_open()) {
    // cerr << "Debug: Não foi possível abrir o arquivo para leitura: " <<
    // filePath << endl;
    return "";
  }
  for (int i = 1; i <= lineNumber; ++i) {
    if (!getline(file, lineContent)) {
      // cerr << "Debug: Falha ao ler a linha " << lineNumber << " de " <<
      // filePath << endl;
      file.close();
      return "";
    }
  }
  file.close();
  return lineContent;
}

/**
 * Escreve (ou sobrescreve) uma string em uma linha específica de um arquivo.
 * Se o número da linha for maior que o número atual de linhas, o arquivo é
 * preenchido com linhas vazias até a linha alvo. filePath O caminho para o
 * arquivo. targetLineNumber O número da linha (1-based) onde o conteúdo deve
 * ser escrito. content A string a ser escrita na linha.
 */
void BPlusTree::writeLineToFile(const string &filePath,
                                int targetLineNumber,
                                const string &content) {
  vector<string> allLines;
  ifstream inFile(filePath);
  string currentLineText;

  // Lê todas as linhas existentes do arquivo.
  if (inFile.is_open()) {
    while (getline(inFile, currentLineText)) {
      allLines.push_back(currentLineText);
    }
    inFile.close();
  }

  if (targetLineNumber < 1)
    return; // Número de linha inválido.

  // Garante que o vetor `allLines` tenha tamanho suficiente.
  while (allLines.size() < static_cast<size_t>(targetLineNumber)) {
    allLines.push_back("");
  }
  allLines[targetLineNumber - 1] = content; // Define o conteúdo da linha alvo.

  // Escreve todas as linhas de volta no arquivo, sobrescrevendo-o.
  ofstream outFile(filePath, ios::trunc);
  if (outFile.is_open()) {
    for (size_t i = 0; i < allLines.size(); ++i) {
      // Evita adicionar uma nova linha extra se a última linha for vazia e for
      // realmente a última.
      outFile << allLines[i]
              << (i == allLines.size() - 1 && allLines[i].empty() &&
                          targetLineNumber == static_cast<int>(allLines.size())
                      ? ""
                      : "\n");
    }
    outFile.close();
  } else {
    cerr
        << "Erro: Não foi possível abrir o arquivo para escrever a linha: "
        << filePath << endl;
  }
}

/**
 * Conta o número de linhas em um arquivo.
 * filePath O caminho para o arquivo.
 * retorna O número de linhas no arquivo, ou 0 se o arquivo não puder ser
 * aberto.
 */
int BPlusTree::countLinesInFile(const string &filePath) {
  ifstream file(filePath);
  if (!file.is_open())
    return 0;
  int lineCount = 0;
  string lineText;
  while (getline(file, lineText)) {
    lineCount++;
  }
  file.close();
  return lineCount;
} 

/**
 *  Carrega um nó do arquivo de índice a partir do seu ID (número da linha).
 * A linha lida do arquivo é então passada para `parseNodeString` * objeto Node carregado, ou nullptr se o ID for 0, a linha estiver vazia ou
 * houver erro no parsing.
 */
Node *BPlusTree::loadNodeFromFile(int nodeIdToLoad) {
  if (nodeIdToLoad == 0)
    return nullptr;
  // Os IDs dos nós no arquivo de índice são armazenados após as linhas de
  // cabeçalho.
  string nodeString =
      readLineFromFile(indexFilePath, nodeIdToLoad + HEADER_LINES);
  if (nodeString.empty()) {
    // cerr << "Debug: String vazia ao carregar nó ID " << nodeIdToLoad <<
    // endl;
    return nullptr;
  }
  return parseNodeString(nodeString, nodeIdToLoad);
}

/**
 * Salva um nó (representado pelo objeto Node) no arquivo de índice.
 * O nó é formatado como uma string por `formatNodeString` e escrito na linha
 * correspondente ao seu ID. nodeToSave Ponteiro para o objeto Node a ser salvo.
 */
void BPlusTree::saveNodeToFile(Node *nodeToSave) {
  if (!nodeToSave || nodeToSave->id == 0) {
    cerr << "Erro: Não é possível salvar nó nulo ou nó sem ID."
              << endl;
    return;
  }
  string nodeStr = formatNodeString(nodeToSave);
  // Os IDs dos nós no arquivo de índice são armazenados após as linhas de
  // cabeçalho.
  writeLineToFile(indexFilePath, nodeToSave->id + HEADER_LINES, nodeStr);
}

/**
 * Analisa (faz parsing) uma string lida do arquivo de índice para reconstruir
 * um objeto Node. A string deve estar no formato:
 * "T;numChaves;chave1;...;chaveN;ponteiro1;...;ponteiroN[;ant;prox]" para
 * folhas ou "T;numChaves;chave1;...;chaveN;filhoID1;...;filhoID(N+1)" para nós
 * internos. line A string contendo a representação do nó. nodeIdFromFile O ID
 * do nó, conforme lido do arquivo (usado para consistência). retorna Ponteiro
 * para o objeto Node reconstruído, ou nullptr se a string estiver vazia ou
 * houver erro no parsing.
 */
Node *BPlusTree::parseNodeString(const string &line, int nodeIdFromFile) {
  if (line.empty())
    return nullptr;
  stringstream ss(line);
  string segment;
  char typeChar;
  int numKeysInString;

  // Lê o tipo do nó (L para folha, I para interno).
  if (!getline(ss, segment, ';')) { /*delete currentIndexNodeInRam;
                                            currentIndexNodeInRam = nullptr;*/
    return nullptr;
  }
  if (segment.length() !=
      1) { /*delete currentIndexNodeInRam; currentIndexNodeInRam = nullptr;*/
    return nullptr;
  }
  typeChar = segment[0];

  Node *parsedNode = new Node(treeOrder, (typeChar == 'L'), nodeIdFromFile);

  // Lê o número de chaves.
  if (!getline(ss, segment, ';')) {
    delete parsedNode;
    return nullptr;
  }
  try {
    numKeysInString = stoi(segment);
  } catch (const exception &) {
    delete parsedNode;
    return nullptr;
  }
  parsedNode->numKeys = numKeysInString;

  // Lê as chaves.
  parsedNode->keys.resize(parsedNode->numKeys);
  for (int i = 0; i < parsedNode->numKeys; ++i) {
    if (!getline(ss, segment, ';')) {
      delete parsedNode;
      return nullptr;
    }
    try {
      parsedNode->keys[i] = stoi(segment);
    } catch (const exception &) {
      delete parsedNode;
      return nullptr;
    }
  }

  if (parsedNode->isLeaf) {
    // Se for folha, lê os ponteiros de dados.
    parsedNode->dataPointers.resize(parsedNode->numKeys);
    for (int i = 0; i < parsedNode->numKeys; ++i) {
      if (!getline(ss, segment, ';')) {
        delete parsedNode;
        return nullptr;
      }
      try {
        parsedNode->dataPointers[i] = stoi(segment);
      } catch (const exception &) {
        delete parsedNode;
        return nullptr;
      }
    }
    // Lê os IDs dos nós folha vizinhos (anterior e próximo).
    if (!getline(ss, segment, ';')) {
      delete parsedNode;
      return nullptr;
    }
    try {
      parsedNode->prevLeafId = stoi(segment);
    } catch (const exception &) {
      delete parsedNode;
      return nullptr;
    }

    if (!getline(ss, segment, ';')) {
      delete parsedNode;
      return nullptr;
    }
    try {
      parsedNode->nextLeafId = stoi(segment);
    } catch (const exception &) {
      delete parsedNode;
      return nullptr;
    }
  } else { // Nó Interno
    // Se for interno, lê os IDs dos nós filhos.
    int numChildren = parsedNode->numKeys + 1;
    parsedNode->childNodeIds.resize(numChildren);
    for (int i = 0; i < numChildren; ++i) {
      if (!getline(ss, segment, ';')) {
        delete parsedNode;
        return nullptr;
      }
      try {
        parsedNode->childNodeIds[i] = stoi(segment);
      } catch (const exception &) {
        delete parsedNode;
        return nullptr;
      }
    }
  }
  return parsedNode;
}

/**
 * Formata um objeto Node em uma representação de string para persistência no
 * arquivo de índice. O formato é o mesmo esperado por `parseNodeString`. node
 * Ponteiro para o objeto Node a ser formatado. retorna String contendo a
 * representação formatada do nó, ou string vazia se o nó for nulo.
 */
string BPlusTree::formatNodeString(Node *node) {
  if (!node)
    return "";
  stringstream ss;
  ss << (node->isLeaf ? 'L' : 'I') << ";";
  ss << node->numKeys << ";";
  for (int i = 0; i < node->numKeys; ++i) {
    ss << node->keys[i] << ";";
  }
  if (node->isLeaf) {
    for (int i = 0; i < node->numKeys; ++i) {
      ss << node->dataPointers[i] << ";";
    }
    ss << node->prevLeafId << ";";
    ss << node->nextLeafId << ";";
  } else {
    for (size_t i = 0; i < node->childNodeIds.size(); ++i) {
      ss << node->childNodeIds[i] << ";";
    }
  }
  return ss.str();
}

/**
 * Insere uma chave e seu ponteiro de dados associado na árvore B+.
 * Se a árvore estiver vazia, cria um novo nó raiz (folha).
 * Caso contrário, encontra o nó folha apropriado para inserção.
 * Se o nó folha tiver espaço, insere diretamente.
 * Se o nó folha estiver cheio, realiza a divisão (split) do nó e insere a
 * chave. key A chave a ser inserida (ano_colheita). dataRecordId O ponteiro
 * para o registro de dados (número da linha em vinhos.csv).
 */
void BPlusTree::insert(int key, int dataRecordId) {
  // Se a árvore está vazia (sem raiz), cria uma nova raiz que é uma folha.
  if (rootNodeId == 0) {
    Node *newRoot =
        createNewBufferedNode(true); // Cria nova folha e a torna raiz.
    rootNodeId = newRoot->id;
    newRoot->keys.push_back(key);
    newRoot->dataPointers.push_back(dataRecordId);
    newRoot->numKeys = 1;
    markCurrentNodeDirty(); // Marca para salvar no destrutor ou quando o buffer
                            // for usado por outro nó.
    return;
  }

  vector<int>
      pathNodeIds; // Vetor para armazenar o caminho da raiz até a folha.
  int leafNodeId = findLeafNodeIdToInsert(
      key, pathNodeIds); // Encontra o ID da folha para inserção.

  if (leafNodeId == 0) {
    cerr
        << "Erro: Não foi possível encontrar o ID do nó folha para inserção."
        << endl;
    return;
  }

  Node *leafNode = accessNode(leafNodeId); // Carrega a folha para o buffer.
  if (!leafNode) {
    cerr << "Erro: Não foi possível acessar o ID do nó folha "
              << leafNodeId << " para inserção." << endl;
    return;
  }

  // Verifica se a folha está cheia.
  if (leafNode->isFull()) {
    splitAndInsertLeaf(leafNodeId, key, dataRecordId,
                       pathNodeIds); // Divide a folha e insere.
  } else {
    insertIntoLeafNonFull(leafNodeId, key,
                          dataRecordId); // Insere na folha (não cheia).
  }
}

/**
 * Encontra o ID do nó folha onde uma nova chave deve ser inserida.
 * Percorre a árvore da raiz até um nó folha, seguindo os ponteiros apropriados
 * com base na chave. key A chave a ser inserida. pathNodeIds Vetor de
 * referência que será preenchido com os IDs dos nós no caminho da raiz até a
 * folha. retorna O ID do nó folha encontrado, ou 0 se a árvore estiver vazia ou
 * ocorrer um erro.
 */
int BPlusTree::findLeafNodeIdToInsert(int key, vector<int> &pathNodeIds) {
  pathNodeIds.clear();
  if (rootNodeId == 0)
    return 0; // Árvore vazia.

  int currentNodeId = rootNodeId;
  pathNodeIds.push_back(currentNodeId); // Adiciona a raiz ao caminho.
  Node *tempNode =
      accessNode(currentNodeId); // Carrega o nó atual para o buffer.

  // Percorre a árvore enquanto o nó atual não for uma folha.
  while (tempNode != nullptr && !tempNode->isLeaf) {
    // Encontra o ponteiro para o filho correto usando lower_bound nas chaves do
    // nó interno.
    auto it = lower_bound(tempNode->keys.begin(),
                               tempNode->keys.begin() + tempNode->numKeys, key);
    int childIdx = distance(tempNode->keys.begin(), it);

    // Validação do índice do filho.
    if (childIdx >= static_cast<int>(tempNode->childNodeIds.size()) ||
        tempNode->childNodeIds[childIdx] == 0) {
      cerr
          << "Erro: Índice de filho inválido ou ID de filho nulo no nó interno "
          << tempNode->id << endl;
      return 0;
    }
    currentNodeId = tempNode->childNodeIds[childIdx]; // Move para o filho.
    pathNodeIds.push_back(currentNodeId); // Adiciona o filho ao caminho.
    tempNode = accessNode(currentNodeId); // Carrega o próximo nó.
  }

  if (tempNode == nullptr)
    return 0;          // Erro ao acessar um nó no caminho.
  return tempNode->id; // Retorna o ID do nó folha encontrado.
}

/**
 * Insere uma chave e um ponteiro de dados em um nó folha que não está cheio.
 * A chave e o ponteiro são inseridos na posição correta para manter a ordem das
 * chaves. leafNodeId O ID do nó folha onde a inserção ocorrerá. key A chave a
 * ser inserida. dataRecordId O ponteiro de dados associado à chave.
 */
void BPlusTree::insertIntoLeafNonFull(int leafNodeId, int key,
                                      int dataRecordId) {
  Node *leaf = accessNode(leafNodeId); // Carrega a folha para o buffer.
  if (!leaf || !leaf->isLeaf || leaf->isFull()) {
    cerr << "Erro: Não é possível inserir em nó não folha, folha cheia ou "
                 "folha nula."
              << endl;
    return;
  }

  // Encontra a posição correta para inserir a nova chave, mantendo a ordem.
  auto it = lower_bound(leaf->keys.begin(), leaf->keys.begin() + leaf->numKeys, key);
  int insertPos = distance(leaf->keys.begin(), it);

  // Insere a chave e o ponteiro de dados.
  leaf->keys.insert(leaf->keys.begin() + insertPos, key);
  leaf->dataPointers.insert(leaf->dataPointers.begin() + insertPos,
                            dataRecordId);
  leaf->numKeys++;
  markCurrentNodeDirty(); // Marca a folha como suja.
}

/**
 * Realiza a divisão (split) de um nó folha cheio e insere a nova chave e
 * ponteiro de dados. Cria um novo nó folha, distribui as chaves e ponteiros
 * entre o nó antigo e o novo. Atualiza os ponteiros de vizinhança (prevLeafId,
 * nextLeafId) entre as folhas. Promove a primeira chave do novo nó folha para
 * ser inserida no nó pai. leafNodeId O ID do nó folha a ser dividido.
 * keyToInsert A chave a ser inserida.
 * dataPtrToInsert O ponteiro de dados associado à chave a ser inserida.
 * pathNodeIds O caminho da raiz até o nó pai da folha que está sendo dividida.
 */
void BPlusTree::splitAndInsertLeaf(int leafNodeId, int keyToInsert,
                                   int dataPtrToInsert,
                                   vector<int> &pathNodeIds) {
  Node *leaf =
      accessNode(leafNodeId); // Carrega a folha original para o buffer.
  if (!leaf)
    return;

  // Cria vetores temporários com todas as chaves e ponteiros (incluindo o novo
  // par).
  vector<int> tempKeys = leaf->keys;
  vector<int> tempDataPointers = leaf->dataPointers;
  auto it_k = lower_bound(tempKeys.begin(), tempKeys.end(), keyToInsert);
  int insertPos = distance(tempKeys.begin(), it_k);
  tempKeys.insert(tempKeys.begin() + insertPos, keyToInsert);
  tempDataPointers.insert(tempDataPointers.begin() + insertPos,
                          dataPtrToInsert);

  // Cria um novo nó folha (este será o nó da direita após a divisão).
  Node *newLeafBufferPtr = createNewBufferedNode(true);
  int newLeafId = newLeafBufferPtr->id;

  leaf =
      accessNode(leafNodeId); // Reacessa o nó folha original (pode ter sido
                              // ejetado do buffer por createNewBufferedNode).
  if (!leaf) {
    return;
  }

  // Calcula quantos itens vão para cada nó após a divisão.
  int numItemsInNewLeaf = ceil(static_cast<double>(tempKeys.size()) / 2.0);
  int numItemsInOldLeaf = tempKeys.size() - numItemsInNewLeaf;

  // Atualiza o nó folha original (nó da esquerda).
  leaf->keys.assign(tempKeys.begin(), tempKeys.begin() + numItemsInOldLeaf);
  leaf->dataPointers.assign(tempDataPointers.begin(),
                            tempDataPointers.begin() + numItemsInOldLeaf);
  leaf->numKeys = numItemsInOldLeaf;
  markCurrentNodeDirty();

  // Atualiza o novo nó folha (nó da direita).
  Node *newLeafNodePtr =
      accessNode(newLeafId); // Carrega o novo nó folha para o buffer.
  if (!newLeafNodePtr) {
    return;
  }
  newLeafNodePtr->keys.assign(tempKeys.begin() + numItemsInOldLeaf,
                              tempKeys.end());
  newLeafNodePtr->dataPointers.assign(
      tempDataPointers.begin() + numItemsInOldLeaf, tempDataPointers.end());
  newLeafNodePtr->numKeys = numItemsInNewLeaf;

  // Atualiza os ponteiros de vizinhança.
  newLeafNodePtr->nextLeafId =
      leaf->nextLeafId; // O próximo do novo é o antigo próximo do original.
  newLeafNodePtr->prevLeafId = leaf->id; // O anterior do novo é o nó original.
  markCurrentNodeDirty();

  leaf = accessNode(leafNodeId); // Reacessa o nó folha original.
  if (!leaf) {
    return;
  }
  leaf->nextLeafId = newLeafId; // O próximo do original agora é o novo nó.
  markCurrentNodeDirty();

  // Se o novo nó folha tem um vizinho à direita, atualiza o ponteiro `prev`
  // desse vizinho.
  if (newLeafNodePtr->nextLeafId != 0) {
    Node *nextNextLeaf = accessNode(newLeafNodePtr->nextLeafId);
    if (nextNextLeaf) {
      nextNextLeaf->prevLeafId = newLeafId;
      markCurrentNodeDirty();
    }
  }

  // A primeira chave do novo nó folha (da direita) é promovida para o pai.
  int keyToPushUp = newLeafNodePtr->keys[0];
  pathNodeIds.pop_back(); // Remove o ID da folha dividida do caminho, o último
                          // ID no path é o pai.
  insertIntoParent(leafNodeId, keyToPushUp, newLeafId, pathNodeIds);
}

/**
 * Insere uma chave e um ponteiro para um novo filho em um nó pai.
 * Isso ocorre após a divisão (split) de um nó filho (folha ou interno).
 * Se o nó pai tiver espaço, a chave e o ponteiro são inseridos diretamente.
 * Se o nó pai estiver cheio, ele também é dividido, e o processo continua
 * recursivamente para cima. oldChildNodeId O ID do filho original (à esquerda
 * da chave promovida). keyToPushUp A chave promovida da divisão do filho.
 * newChildNodeId O ID do novo filho criado na divisão (à direita da chave
 * promovida). pathNodeIds O caminho da raiz até o avô do nó que foi
 * originalmente dividido (ou vazio se o pai é a raiz).
 */
void BPlusTree::insertIntoParent(int oldChildNodeId, int keyToPushUp,
                                 int newChildNodeId,
                                 vector<int> &pathNodeIds) {
  // Se pathNodeIds está vazio, significa que o nó dividido era a raiz, ou o pai
  // da folha dividida era a raiz. Neste caso, uma nova raiz precisa ser criada.
  if (pathNodeIds.empty()) {
    createNewRootAndUpdate(oldChildNodeId, keyToPushUp, newChildNodeId);
    return;
  }

  int parentNodeId = pathNodeIds.back(); // Pega o ID do nó pai.
  pathNodeIds.pop_back(); // Remove o pai do caminho para a próxima chamada
                          // recursiva, se houver.
  Node *parent = accessNode(parentNodeId); // Carrega o pai para o buffer.
  if (!parent) {
    cerr << "Erro: Não foi possível acessar o ID do nó pai "
              << parentNodeId << endl;
    return;
  }

  // Encontra a posição correta para inserir a chave promovida no nó pai.
  auto it = lower_bound(parent->keys.begin(),
parent->keys.begin() + parent->numKeys, keyToPushUp);
  int insertPos = distance(parent->keys.begin(), it);

  // Se o pai não está cheio, insere a chave e o novo filho.
  if (parent->numKeys < treeOrder - 1) {
    parent->keys.insert(parent->keys.begin() + insertPos, keyToPushUp);
    parent->childNodeIds.insert(parent->childNodeIds.begin() + insertPos + 1,
                                newChildNodeId);
    parent->numKeys++;
    markCurrentNodeDirty();
  } else { // O pai está cheio, precisa ser dividido.
    // Cria vetores temporários com todas as chaves e filhos (incluindo o novo
    // par).
    vector<int> tempKeys = parent->keys;
    vector<int> tempChildren = parent->childNodeIds;
    tempKeys.insert(tempKeys.begin() + insertPos, keyToPushUp);
    tempChildren.insert(tempChildren.begin() + insertPos + 1, newChildNodeId);

    // Cria um novo nó interno.
    Node *newInternalBufferPtr = createNewBufferedNode(false);
    int newInternalNodeId = newInternalBufferPtr->id;

    parent = accessNode(parentNodeId); // Reacessa o pai.
    if (!parent) {
      return;
    }

    // Calcula o ponto de divisão e a chave a ser promovida para o próximo
    // nível.
    int internalSplitPointKeyIdx =
        (treeOrder - 1) / 2; // Chave do meio que será promovida.
    int keyToPushFurtherUp = tempKeys[internalSplitPointKeyIdx];

    // Atualiza o nó pai original (nó da esquerda).
    parent->keys.assign(tempKeys.begin(),
                        tempKeys.begin() + internalSplitPointKeyIdx);
    parent->childNodeIds.assign(tempChildren.begin(),
                                tempChildren.begin() +
                                    internalSplitPointKeyIdx + 1);
    parent->numKeys = internalSplitPointKeyIdx;
    markCurrentNodeDirty();

    // Atualiza o novo nó interno (nó da direita).
    Node *newInternalNodePtr =
        accessNode(newInternalNodeId); // Carrega o novo nó interno.
    if (!newInternalNodePtr) {
      return;
    }
    newInternalNodePtr->keys.assign(
        tempKeys.begin() + internalSplitPointKeyIdx + 1, tempKeys.end());
    newInternalNodePtr->childNodeIds.assign(tempChildren.begin() +
                                                internalSplitPointKeyIdx + 1,
                                            tempChildren.end());
    newInternalNodePtr->numKeys =
        tempKeys.size() - (internalSplitPointKeyIdx + 1);
    markCurrentNodeDirty();

    // Chama recursivamente para inserir a `keyToPushFurtherUp` no avô.
    insertIntoParent(parentNodeId, keyToPushFurtherUp, newInternalNodeId,
                     pathNodeIds);
  }
}

/**
 * Cria uma nova raiz para a árvore.
 * Isso ocorre quando uma divisão (split) propaga até a raiz anterior, ou quando
 * a primeira raiz é criada. A nova raiz terá dois filhos: o `oldLeftChildId` e
 * o `oldRightChildId`, separados pela `key`. oldLeftChildId O ID do filho à
 * esquerda da chave na nova raiz. key A chave que separará os dois filhos na
 * nova raiz. oldRightChildId O ID do filho à direita da chave na nova raiz.
 */
void BPlusTree::createNewRootAndUpdate(int oldLeftChildId, int key,
                                       int oldRightChildId) {
  Node *newRoot =
      createNewBufferedNode(false); // Cria um novo nó interno para ser a raiz.
  rootNodeId = newRoot->id;         // Atualiza o ID da raiz da árvore.
  newRoot->keys.push_back(key);
  newRoot->childNodeIds.push_back(oldLeftChildId);
  newRoot->childNodeIds.push_back(oldRightChildId);
  newRoot->numKeys = 1;
  markCurrentNodeDirty();
}

/**
 * Busca por uma chave na árvore B+.
 * key A chave a ser buscada.
 * retorna vetor de inteiros contendo os ponteiros de dados (números de linha em
 * vinhos.csv) associados à chave. retorna um vetor vazio se a chave não for
 * encontrada ou a árvore estiver vazia.
 */
vector<int> BPlusTree::search(int key) {
  vector<int> resultRecordIds;
  if (rootNodeId == 0)
    return resultRecordIds; // Árvore vazia.

  vector<int>
      path; // Não usado aqui, mas findLeafNodeIdToInsert o preenche.
  int leafNodeId = findLeafNodeIdToInsert(
      key, path); // Encontra a folha onde a chave deveria estar.
  if (leafNodeId == 0)
    return resultRecordIds; // Chave não pode estar na árvore.

  Node *leafNode = accessNode(leafNodeId); // Carrega a folha para o buffer.
  if (!leafNode)
    return resultRecordIds;

  // Procura a chave na folha.
  auto it = lower_bound(leafNode->keys.begin(),
                             leafNode->keys.begin() + leafNode->numKeys, key);
  int keyPos = distance(leafNode->keys.begin(), it);

  // Itera pela folha e pelas folhas seguintes (se necessário) para coletar
  // todos os dataPointers da chave.
  while (leafNode != nullptr && keyPos < leafNode->numKeys &&
         leafNode->keys[keyPos] == key) {
    resultRecordIds.push_back(leafNode->dataPointers[keyPos]);
    keyPos++;
    // Se chegou ao fim das chaves na folha atual, tenta ir para a próxima
    // folha.
    if (keyPos >= leafNode->numKeys) {
      int nextLeafIdToSearch = leafNode->nextLeafId;
      if (nextLeafIdToSearch == 0) {
        leafNode = nullptr;
        break;
      }                                          // não há mais folhas
      leafNode = accessNode(nextLeafIdToSearch); // carrega a próxima folha
      if (!leafNode)
        break;    // erro ao carregar próxima folha
      keyPos = 0; // começa do início da próxima folha
      // se a primeira chave da próxima folha não for a chave buscada, para
      if (leafNode->numKeys == 0 || leafNode->keys[0] != key) {
        leafNode = nullptr;
        break;
      }
    }
  }
  return resultRecordIds;
}

/**
 * imprime a estrutura da árvore B+ para fins de depuração
 * mostra a ordem da árvore, o ID da raiz e o próximo ID de nó disponível
 * em seguida, chama `printNodeRecursive` para imprimir os nós recursivamente
 * gerencia o buffer de nó para não interferir na impressão
 */
void BPlusTree::printTreeForDebug() {
  cout << "Estrutura da Árvore B+ (Ordem: " << treeOrder << ")"
            << endl;
  if (rootNodeId == 0) {
    cout << "  Árvore está vazia." << endl;
    return;
  }
  cout << "  ID do Nó Raiz: " << rootNodeId << endl;
  cout << "  Contador do Próximo ID de Nó para novos nós: "
            << nextNodeIdCounter << endl;

  // salva o estado do buffer de nó atual para restaurá-lo após a impressão
  Node *tempSavedNode = nullptr;
  int tempSavedId = 0;
  bool tempSavedDirty = false;

  if (currentIndexNodeInRam) {
    if (currentIndexNodeDirty) {
      saveNodeToFile(currentIndexNodeInRam); // salva se estiver sujo
    }
    // copia os dados do buffer atual para variáveis temporárias
    tempSavedNode = currentIndexNodeInRam;
    tempSavedId = currentIndexNodeInRamId;
    tempSavedDirty = currentIndexNodeDirty;
    // limpa o buffer para que printNodeRecursive possa usar livremente
    currentIndexNodeInRam = nullptr;
    currentIndexNodeInRamId = 0;
    currentIndexNodeDirty = false;
  }

  printNodeRecursive(rootNodeId,
                     0); // inicia a impressão recursiva a partir da raiz

  // restaura o estado do buffer de nó
  if (tempSavedNode) {
    // se printNodeRecursive deixou algo sujo no buffer, salva
    if (currentIndexNodeInRam && currentIndexNodeDirty)
      saveNodeToFile(currentIndexNodeInRam);
    delete currentIndexNodeInRam; // libera o que printNodeRecursive possa ter
                                  // deixado

    currentIndexNodeInRam = tempSavedNode; // restaura o nó original
    currentIndexNodeInRamId = tempSavedId;
    if (currentIndexNodeInRam)
      currentIndexNodeInRam->id =
          tempSavedId; // garante que o ID no objeto nó esteja correto
    currentIndexNodeDirty = tempSavedDirty;
  }
}

/**
 *  Função auxiliar recursiva para imprimir os detalhes de um nó e seus filhos
 * Usada por `printTreeForDebug`
 * nodeIdToPrint O ID do nó a ser impresso
 * level O nível de profundidade do nó na árvore (usado para indentação)
 */
void BPlusTree::printNodeRecursive(int nodeIdToPrint, int level) {
  if (nodeIdToPrint == 0)
    return;
  Node *node = accessNode(nodeIdToPrint); // carrega o nó para o buffer
  if (!node) {
    cout << string(level * 2, ' ')
              << "[Erro ao carregar Nó ID: " << nodeIdToPrint << "]"
              << endl;
    return;
  }

  cout << string(level * 2, ' '); // indentação
  cout << "[" << (node->isLeaf ? 'L' : 'I') << ":" << node->id
            << "] Chaves: (";
  for (int i = 0; i < node->numKeys; ++i) {
    cout << node->keys[i] << (i == node->numKeys - 1 ? "" : ",");
  }
  cout << ")";

  if (node->isLeaf) {
    cout << " PonteirosDados: (";
    for (int i = 0; i < node->numKeys; ++i) {
      cout << node->dataPointers[i] << (i == node->numKeys - 1 ? "" : ",");
    }
    cout << ") Ant: " << node->prevLeafId << " Prox: " << node->nextLeafId;
  }
  cout << endl;

  // se não for folha, imprime recursivamente os filhos
  if (!node->isLeaf) {
    vector<int> children = node->childNodeIds;
    for (int child_id : children) {
      if (child_id !=
          0) { // add verificação para não tentar imprimir filho com ID 0
        printNodeRecursive(child_id, level + 1);
      }
    }
  }
}
