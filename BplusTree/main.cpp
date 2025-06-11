#include "bplustree.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
// Função para analisar uma linha CSV
vector<string> parseCSVLine(const string &line) {
  vector<string> result;
  stringstream ss(line);
  string segment;
  while (getline(ss, segment, ',')) { // Corrigido: Usar literal char ","
    result.push_back(segment);
  }
  return result;
}

// função para encontrar números de linha em vinhos.csv para um dado
// ano_colheita
vector<int> findRecordLineNumbers(const string &csvFilePath,
                                       int ano_colheita_to_find) {
  vector<int> lineNumbers;
  ifstream csvFile(csvFilePath);
  string line;
  int currentLineNumber = 0;

  if (!csvFile.is_open()) {
    cerr << "Erro: Não foi possível abrir o arquivo de dados: "
              << csvFilePath << endl;
    return lineNumbers;
  }

  // pula cabeçalho
  if (getline(csvFile, line)) {
    currentLineNumber++;
  }

  while (getline(csvFile, line)) {
    currentLineNumber++;
    vector<string> parsed = parseCSVLine(line);
    if (parsed.size() == 4) { // verifica se tem 4 colunas como esperado
      try {
        int ano =
            stoi(parsed[2]); // ano_colheita é a terceira coluna (índice 2)
        if (ano == ano_colheita_to_find) {
          lineNumbers.push_back(currentLineNumber);
        }
      } catch (const exception &e) {
        // cerr << "Aviso: Não foi possível analisar ano_colheita na linha
        // " << currentLineNumber << ": " << parsed[2] << endl;
      }
    }
  }
  csvFile.close();
  return lineNumbers;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    cerr << "Uso: " << argv[0] << " <caminho_do_arquivo_de_entrada>"
              << endl;
    return 1;
  }

  string inputFilePath = argv[1];
  ifstream inputFile(inputFilePath);
  string line;

  if (!inputFile.is_open()) {
    cerr << "Erro: Não foi possível abrir o arquivo de entrada: "
              << inputFilePath << endl;
    return 1;
  }

  int order = 0;
  // lê a primeira linha para obter a ordem da árvore
  if (getline(inputFile, line)) {
    if (line.rfind("FLH/", 0) == 0) { // verifica se a linha começa com "FLH/"
      try {
        order = stoi(line.substr(4)); // pega o número após "FLH/"
      } catch (const exception &e) {
        cerr << "Erro: Formato de ordem inválido na primeira linha: "
                  << line << endl;
        return 1;
      }
    } else {
      cerr << "Erro: A primeira linha deve estar no formato FLH/<ordem>. "
                   "Recebido: "
                << line << endl;
      return 1;
    }
  } else {
    cerr << "Erro: Arquivo de entrada vazio ou não foi possível ler a "
                 "primeira linha."
              << endl;
    return 1;
  }

  if (order <= 2) { // a ordem de uma Árvore B+ deve ser pelo menos 3 (isso aqui
                    // é propriedade da arvore b+ ein marcos jr)
    cerr << "Erro: A ordem da Árvore B+ deve ser pelo menos 3. Recebido: "
              << order << endl;
    return 1;
  }

  string indexFileName = "bplus_tree_index.txt";
  string dataFileName = "vinhos.csv"; // nome do arquivo de dados

  // verifica se vinhos.csv existe no diretório atual, se não, copia de
  // /home/ubuntu/upload/
  ifstream checkDataFile(dataFileName);
  if (!checkDataFile.good()) {
    ifstream src("/home/ubuntu/upload/vinhos.csv", ios::binary);
    if (!src.is_open()) {
      cerr
          << "Erro: Não foi possível abrir o vinhos.csv de origem para cópia."
          << endl;
    } else {
      ofstream dst(dataFileName, ios::binary);
      if (!dst.is_open()) {
        cerr << "Erro: Não foi possível abrir o vinhos.csv de destino "
                     "para cópia."
                  << endl;
      } else {
        dst << src.rdbuf();
        cout << "Copiado vinhos.csv para o diretório atual." << endl;
      }
      src.close();
      dst.close();
    }
  }
  checkDataFile.close();

  BPlusTree bTree(order, indexFileName, dataFileName);

  // processa os comandos restantes do arquivo de entrada
  while (getline(inputFile, line)) {
    if (line.empty() || line[0] == '#') { // ignora linhas vazias ou comentários
      continue;
    }

    string command_full = line;
    size_t colon_pos = command_full.find(":");
    string command_type;
    string command_value_str;

    if (colon_pos != string::npos) {
      command_type = command_full.substr(0, colon_pos);
      command_value_str = command_full.substr(colon_pos + 1);
    } else {
      cerr << "Aviso: Comando malformado (sem dois pontos): " << line
                << endl;
      continue;
    }

    if (command_type == "INC") {
      try {
        int key = stoi(command_value_str);
        // encontra todos os registros com este ano_colheita para obter seus
        // números de linha
        vector<int> recordLines = findRecordLineNumbers(dataFileName, key);
        if (recordLines.empty()) {
          // cout << "  nenhum registro encontrado em " << dataFileName <<
          // " para ano_colheita = " << key << endl;
        } else {
          for (int recLine : recordLines) {
            bTree.insert(key, recLine);
          }
        }
      } catch (const exception &e) {
        cerr << "Erro ao analisar comando INC: " << line << " - "
                  << e.what() << endl;
      }
    } else if (command_type == "BUS=") {
      try {
        int key = stoi(command_value_str);
        vector<int> results = bTree.search(key);
        if (results.empty()) {
          cout << "CHAVE NAO ENCONTRADA: " << key << endl;
        } else {
          cout << "CHAVE ENCONTRADA: " << key << " LINHAS: ";
          sort(results.begin(),
                    results.end()); // ordena para saída consistente
          for (size_t i = 0; i < results.size(); ++i) {
            cout << results[i] << (i == results.size() - 1 ? "" : ",");
          }
          cout << endl;
        }
      } catch (const exception &e) {
        cerr << "Erro ao analisar comando BUS=: " << line << " - "
                  << e.what() << endl;
      }
    } else {
      cerr << "Aviso: Tipo de comando desconhecido: " << command_type
                << " na linha: " << line << endl;
    }
  }

  inputFile.close();
  bTree.printTreeForDebug();
  return 0;
}
