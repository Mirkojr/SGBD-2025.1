#include "bplustree.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Função para analisar uma linha CSV
std::vector<std::string> parseCSVLine(const std::string &line) {
  std::vector<std::string> result;
  std::stringstream ss(line);
  std::string segment;
  while (std::getline(ss, segment, ',')) { // Corrigido: Usar literal char ","
    result.push_back(segment);
  }
  return result;
}

// função para encontrar números de linha em vinhos.csv para um dado
// ano_colheita
std::vector<int> findRecordLineNumbers(const std::string &csvFilePath,
                                       int ano_colheita_to_find) {
  std::vector<int> lineNumbers;
  std::ifstream csvFile(csvFilePath);
  std::string line;
  int currentLineNumber = 0;

  if (!csvFile.is_open()) {
    std::cerr << "Erro: Não foi possível abrir o arquivo de dados: "
              << csvFilePath << std::endl;
    return lineNumbers;
  }

  // pula cabeçalho
  if (std::getline(csvFile, line)) {
    currentLineNumber++;
  }

  while (std::getline(csvFile, line)) {
    currentLineNumber++;
    std::vector<std::string> parsed = parseCSVLine(line);
    if (parsed.size() == 4) { // verifica se tem 4 colunas como esperado
      try {
        int ano =
            std::stoi(parsed[2]); // ano_colheita é a terceira coluna (índice 2)
        if (ano == ano_colheita_to_find) {
          lineNumbers.push_back(currentLineNumber);
        }
      } catch (const std::exception &e) {
        // std::cerr << "Aviso: Não foi possível analisar ano_colheita na linha
        // " << currentLineNumber << ": " << parsed[2] << std::endl;
      }
    }
  }
  csvFile.close();
  return lineNumbers;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Uso: " << argv[0] << " <caminho_do_arquivo_de_entrada>"
              << std::endl;
    return 1;
  }

  std::string inputFilePath = argv[1];
  std::ifstream inputFile(inputFilePath);
  std::string line;

  if (!inputFile.is_open()) {
    std::cerr << "Erro: Não foi possível abrir o arquivo de entrada: "
              << inputFilePath << std::endl;
    return 1;
  }

  int order = 0;
  // lê a primeira linha para obter a ordem da árvore
  if (std::getline(inputFile, line)) {
    if (line.rfind("FLH/", 0) == 0) { // verifica se a linha começa com "FLH/"
      try {
        order = std::stoi(line.substr(4)); // pega o número após "FLH/"
      } catch (const std::exception &e) {
        std::cerr << "Erro: Formato de ordem inválido na primeira linha: "
                  << line << std::endl;
        return 1;
      }
    } else {
      std::cerr << "Erro: A primeira linha deve estar no formato FLH/<ordem>. "
                   "Recebido: "
                << line << std::endl;
      return 1;
    }
  } else {
    std::cerr << "Erro: Arquivo de entrada vazio ou não foi possível ler a "
                 "primeira linha."
              << std::endl;
    return 1;
  }

  if (order <= 2) { // a ordem de uma Árvore B+ deve ser pelo menos 3 (isso aqui
                    // é propriedade da arvore b+ ein marcos jr)
    std::cerr << "Erro: A ordem da Árvore B+ deve ser pelo menos 3. Recebido: "
              << order << std::endl;
    return 1;
  }

  std::string indexFileName = "bplus_tree_index.txt";
  std::string dataFileName = "vinhos.csv"; // nome do arquivo de dados

  // verifica se vinhos.csv existe no diretório atual, se não, copia de
  // /home/ubuntu/upload/
  std::ifstream checkDataFile(dataFileName);
  if (!checkDataFile.good()) {
    std::ifstream src("/home/ubuntu/upload/vinhos.csv", std::ios::binary);
    if (!src.is_open()) {
      std::cerr
          << "Erro: Não foi possível abrir o vinhos.csv de origem para cópia."
          << std::endl;
    } else {
      std::ofstream dst(dataFileName, std::ios::binary);
      if (!dst.is_open()) {
        std::cerr << "Erro: Não foi possível abrir o vinhos.csv de destino "
                     "para cópia."
                  << std::endl;
      } else {
        dst << src.rdbuf();
        std::cout << "Copiado vinhos.csv para o diretório atual." << std::endl;
      }
      src.close();
      dst.close();
    }
  }
  checkDataFile.close();

  BPlusTree bTree(order, indexFileName, dataFileName);

  // processa os comandos restantes do arquivo de entrada
  while (std::getline(inputFile, line)) {
    if (line.empty() || line[0] == '#') { // ignora linhas vazias ou comentários
      continue;
    }

    std::string command_full = line;
    size_t colon_pos = command_full.find(":");
    std::string command_type;
    std::string command_value_str;

    if (colon_pos != std::string::npos) {
      command_type = command_full.substr(0, colon_pos);
      command_value_str = command_full.substr(colon_pos + 1);
    } else {
      std::cerr << "Aviso: Comando malformado (sem dois pontos): " << line
                << std::endl;
      continue;
    }

    if (command_type == "INC") {
      try {
        int key = std::stoi(command_value_str);
        // encontra todos os registros com este ano_colheita para obter seus
        // números de linha
        std::vector<int> recordLines = findRecordLineNumbers(dataFileName, key);
        if (recordLines.empty()) {
          // std::cout << "  nenhum registro encontrado em " << dataFileName <<
          // " para ano_colheita = " << key << std::endl;
        } else {
          for (int recLine : recordLines) {
            bTree.insert(key, recLine);
          }
        }
      } catch (const std::exception &e) {
        std::cerr << "Erro ao analisar comando INC: " << line << " - "
                  << e.what() << std::endl;
      }
    } else if (command_type == "BUS=") {
      try {
        int key = std::stoi(command_value_str);
        std::vector<int> results = bTree.search(key);
        if (results.empty()) {
          std::cout << "CHAVE NAO ENCONTRADA: " << key << std::endl;
        } else {
          std::cout << "CHAVE ENCONTRADA: " << key << " LINHAS: ";
          std::sort(results.begin(),
                    results.end()); // ordena para saída consistente
          for (size_t i = 0; i < results.size(); ++i) {
            std::cout << results[i] << (i == results.size() - 1 ? "" : ",");
          }
          std::cout << std::endl;
        }
      } catch (const std::exception &e) {
        std::cerr << "Erro ao analisar comando BUS=: " << line << " - "
                  << e.what() << std::endl;
      }
    } else {
      std::cerr << "Aviso: Tipo de comando desconhecido: " << command_type
                << " na linha: " << line << std::endl;
    }
  }

  inputFile.close();
  return 0;
}
