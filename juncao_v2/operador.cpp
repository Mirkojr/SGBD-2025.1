#include "operador.h"
#include "pagina.h" // Para MAX_TUPLES_PER_PAGE
#include "utils.h"  // Inclui as funções utilitárias
#include <algorithm>
#include <filesystem> // Para iterar sobre arquivos no diretório
#include <fstream>
#include <functional> // For std::function
#include <iostream>
#include <queue>
#include <sstream>

using namespace std;

namespace fs = std::filesystem;


// Construtor
Operador::Operador(const Tabela &t1, const Tabela &t2, const string &c1,
                   const string &c2)
    : tabela1(t1), tabela2(t2), chave1(c1), chave2(c2) {}

// Métodos getter para os contadores
int Operador::numPagsGeradas() const { return paginasGeradas; }

int Operador::numIOExecutados() const { return IOExecutados; }

int Operador::numTuplasGeradas() const { return tuplasGeradasCount; }

// Implementação de imprimirTuplasGeradas
void Operador::imprimirTuplasGeradas() const {
  cout << "Tuplas Geradas:" << endl;
  for (const auto &tupla : tuplasGeradas) {
    for (size_t i = 0; i < tupla.size(); ++i) {
      cout << tupla[i];
      if (i != tupla.size() - 1)
        cout << " ";
    }
    cout << endl;
  }
}

// Implementação de salvarTuplasGeradas
void Operador::salvarTuplasGeradas(const string &nomeArquivo) const {
  ofstream arquivoSaida(nomeArquivo);
  if (!arquivoSaida.is_open()) {
    cerr << "Erro ao criar o arquivo de saída: " << nomeArquivo << endl;
    return;
  }

  // Escrever cabeçalho (opcional, dependendo do requisito)
  // Para junção, o cabeçalho seria a concatenação dos cabeçalhos das tabelas
  // originais Por simplicidade, vamos pular o cabeçalho aqui.

  for (const auto &tupla : tuplasGeradas) {
    for (size_t i = 0; i < tupla.size(); ++i) {
      arquivoSaida << tupla[i];
      if (i != tupla.size() - 1)
        arquivoSaida << ",";
    }
    arquivoSaida << endl;
  }
  arquivoSaida.close();
  cout << "Tuplas salvas em: " << nomeArquivo << endl;
}

// Implementação de getIndexColuna
int Operador::getIndexColuna(const Tabela &tabela, const string &chave) const {
  for (size_t i = 0; i < tabela.col_names.size(); ++i) {
    if (tabela.col_names[i] == chave) {
      return i;
    }
  }
  return -1; // Coluna não encontrada
}

// Implementação de externalSort
void Operador::externalSort(Tabela &tabela, const string &chave,
                            const string &nomeArquivoSaida) {
  cout << "Iniciando ordenação externa para a tabela: " << tabela.nomeArquivo
       << " pela chave: " << chave << endl;

  int indiceChave = getIndexColuna(tabela, chave);
  if (indiceChave == -1) {
    cerr << "Erro: Chave de ordenação ";

        // Fase 1: Criação de Runs Iniciais
        vector<string>
            runFiles;
    int runCount = 0;
    const int MEMORY_LIMIT_PAGES = 4; // Limite de 4 páginas em memória
    const int PAGES_PER_BLOCK =
        MEMORY_LIMIT_PAGES - 1; // Uma página para saída, 3 para entrada

    string tableDir =
        "data/" + tabela.nomeArquivo.substr(0, tabela.nomeArquivo.find("."));

    // Coletar todos os arquivos de página da tabela
    vector<string> pageFiles;
    for (const auto &entry : fs::directory_iterator(tableDir)) {
      if (entry.is_regular_file() && entry.path().extension() == ".txt") {
        pageFiles.push_back(entry.path().string());
      }
    }
    sort(pageFiles.begin(),
         pageFiles.end()); // Garantir ordem correta das páginas

    string headerLine; // O cabeçalho deve ser lido uma vez da tabela original
                       // ou passado
    // Por simplicidade, vamos assumir que o cabeçalho é o mesmo da tabela
    // original e que ele já foi carregado na Tabela::col_names
    if (!tabela.col_names.empty()) {
      stringstream ss;
      for (size_t i = 0; i < tabela.col_names.size(); ++i) {
        ss << tabela.col_names[i];
        if (i < tabela.col_names.size() - 1)
          ss << ",";
      }
      headerLine = ss.str();
    }

    size_t currentPageIndex = 0;
    while (currentPageIndex < pageFiles.size()) {
      vector<Tupla> currentBlockTuplas;
      for (int i = 0;
           i < PAGES_PER_BLOCK && currentPageIndex < pageFiles.size(); ++i) {
        ifstream pageFileStream(pageFiles[currentPageIndex]);
        if (!pageFileStream.is_open()) {
          cerr << "Erro ao abrir arquivo de página: "
               << pageFiles[currentPageIndex] << endl;
          return;
        }
        Pagina p = lerPaginaDeStream(pageFileStream,
                                     IOExecutados); // Usa a função de utilidade
        for (int j = 0; j < p.qtd_tuplas_ocup; ++j) {
          currentBlockTuplas.push_back(p.tuplas[j]);
        }
        pageFileStream.close();
        currentPageIndex++;
      }

      if (currentBlockTuplas.empty()) {
        break; // Não há mais dados para ler
      }

      // Ordena o bloco em memória
      sort(currentBlockTuplas.begin(), currentBlockTuplas.end(),
           [&](const Tupla &a, const Tupla &b) {
             return getColunaValue(a, indiceChave) <
                    getColunaValue(b, indiceChave);
           });

      // Escreve o run ordenado em um arquivo temporário
      string runFileName =
          tabela.nomeArquivo + "_run_" + to_string(runCount++) + ".tmp";
      ofstream runFileStream(runFileName);
      if (!runFileStream.is_open()) {
        cerr << "Erro ao criar arquivo de run: " << runFileName << endl;
        return;
      }
      // Escrever cabeçalho no run file se a tabela original tinha cabeçalho
      if (!tabela.col_names.empty()) {
        runFileStream << headerLine << endl;
      }

      Pagina currentRunPage;
      for (const auto &tupla : currentBlockTuplas) {
        if (currentRunPage.isFull()) {
          escreverPaginaEmStream(runFileStream, currentRunPage,
                                 IOExecutados); // Usa a função de utilidade
          currentRunPage = Pagina();            // Reseta a página
        }
        currentRunPage.adicionarTupla(tupla);
      }
      if (!currentRunPage.isEmpty()) {
        escreverPaginaEmStream(runFileStream, currentRunPage,
                               IOExecutados); // Escreve a última página do run
      }
      runFileStream.close();
      runFiles.push_back(
          runFileName); // paginasGeradas += (currentBlockTuplas.size() +
                        // MAX_TUPLES_PER_PAGE - 1) / MAX_TUPLES_PER_PAGE// Já
                        // contabilizado em escreverPaginaEmStream
    }

    // Fase 2: Merge dos Runs
    if (runFiles.empty()) {
      cout << "Nenhum run gerado para a tabela " << tabela.nomeArquivo << endl;
      ofstream outFile(nomeArquivoSaida);
      if (!tabela.col_names.empty()) {
        outFile << headerLine << endl;
      }
      outFile.close();
      return;
    }

    if (runFiles.size() == 1) {
      if (rename(runFiles[0].c_str(), nomeArquivoSaida.c_str()) != 0) {
        perror("Erro ao renomear arquivo do run");
      }
      cout << "Tabela " << tabela.nomeArquivo << " ordenada e salva em "
           << nomeArquivoSaida << endl;
      return;
    }

    // K-way merge
    // Comparador para o min-heap
    auto compareTuplas = [&](pair<Tupla, int> a, pair<Tupla, int> b) {
      return getColunaValue(a.first, indiceChave) >
             getColunaValue(b.first, indiceChave);
    };
    priority_queue<pair<Tupla, int>, vector<pair<Tupla, int>>,
                   decltype(compareTuplas)>
        minHeap(compareTuplas);

    vector<ifstream> inputRunFiles(runFiles.size());
    vector<Pagina> currentRunPages(runFiles.size());
    vector<int> currentTupleIndexInPage(runFiles.size(), 0);

    // Abrir todos os arquivos de run e ler a primeira página de cada um
    for (size_t i = 0; i < runFiles.size(); ++i) {
      inputRunFiles[i].open(runFiles[i]);
      if (!inputRunFiles[i].is_open()) {
        cerr << "Erro ao abrir run file para merge: " << runFiles[i] << endl;
        return;
      }
      // Pular cabeçalho se existir
      if (!tabela.col_names.empty()) {
        string tempHeader;
        getline(inputRunFiles[i], tempHeader);
      }

      currentRunPages[i] = lerPaginaDeStream(
          inputRunFiles[i], IOExecutados); // Lê a primeira página
      if (!currentRunPages[i].isEmpty()) {
        minHeap.push({currentRunPages[i].tuplas[0], i});
        currentTupleIndexInPage[i]++;
      } else {
        inputRunFiles[i].close();
      }
    }

    ofstream outputFileStream(nomeArquivoSaida);
    if (!outputFileStream.is_open()) {
      cerr << "Erro ao criar arquivo de saída para merge final: "
           << nomeArquivoSaida << endl;
      return;
    }
    // Escrever cabeçalho no arquivo de saída final
    if (!tabela.col_names.empty()) {
      outputFileStream << headerLine << endl;
    }

    Pagina outputPage; // Página de saída para o arquivo final
    while (!minHeap.empty()) {
      pair<Tupla, int> top = minHeap.top();
      minHeap.pop();

      if (outputPage.isFull()) {
        escreverPaginaEmStream(outputFileStream, outputPage,
                               IOExecutados); // Escreve página cheia
        paginasGeradas++;                     // Contabiliza página gerada
        outputPage = Pagina();                // Reseta a página
      }
      outputPage.adicionarTupla(top.first);

      int runIndex = top.second;
      if (currentTupleIndexInPage[runIndex] <
          currentRunPages[runIndex].qtd_tuplas_ocup) {
        // Ainda há tuplas na página atual do run
        minHeap.push({currentRunPages[runIndex]
                          .tuplas[currentTupleIndexInPage[runIndex]],
                      runIndex});
        currentTupleIndexInPage[runIndex]++;
      } else if (inputRunFiles[runIndex].is_open()) {
        // A página atual do run terminou, tentar ler a próxima página
        currentRunPages[runIndex] =
            lerPaginaDeStream(inputRunFiles[runIndex], IOExecutados);
        currentTupleIndexInPage[runIndex] = 0;
        if (!currentRunPages[runIndex].isEmpty()) {
          minHeap.push({currentRunPages[runIndex].tuplas[0], runIndex});
          currentTupleIndexInPage[runIndex]++;
        } else {
          inputRunFiles[runIndex].close(); // Fim do run
        }
      }
    }
    if (!outputPage.isEmpty()) {
      escreverPaginaEmStream(outputFileStream, outputPage,
                             IOExecutados); // Escreve a última página
      paginasGeradas++;                     // Contabiliza página gerada
    }
    outputFileStream.close();

    // Remover arquivos de run temporários
    for (const string &runFile : runFiles) {
      remove(runFile.c_str());
    }

    cout << "Tabela " << tabela.nomeArquivo << " ordenada e salva em "
         << nomeArquivoSaida << endl;
  }

  // Implementação de mergeJoin
  void Operador::mergeJoin(const string &arq1Ordenado,
                           const string &arq2Ordenado) {
    cout << "Iniciando Merge Join entre " << arq1Ordenado << " e "
         << arq2Ordenado << endl;

    ifstream file1(arq1Ordenado);
    ifstream file2(arq2Ordenado);

    if (!file1.is_open()) {
      cerr << "Erro ao abrir arquivo ordenado: " << arq1Ordenado << endl;
      return;
    }
    if (!file2.is_open()) {
      cerr << "Erro ao abrir arquivo ordenado: " << arq2Ordenado << endl;
      return;
    }

    string header1, header2;

    // Ler cabeçalhos
    if (!tabela1.col_names.empty())
      getline(file1, header1);
    if (!tabela2.col_names.empty())
      getline(file2, header2);

    int idxChave1 = getIndexColuna(tabela1, chave1);
    int idxChave2 = getIndexColuna(tabela2, chave2);

    if (idxChave1 == -1 || idxChave2 == -1) {
      cerr << "Erro: Chave de junção não encontrada em uma das tabelas."
           << endl;
      file1.close();
      file2.close();
      return;
    }

    // Posições para "rebobinar" file2 em caso de duplicatas na tabela1
    streampos file2_pos_before_match;
    string file2_linha_before_match;

    Pagina currentPagina1 = lerPaginaDeStream(file1, IOExecutados);
    Pagina currentPagina2 = lerPaginaDeStream(file2, IOExecutados);

    int idxTupla1 = 0;
    int idxTupla2 = 0;

    Pagina outputPage; // Página de saída para o resultado da junção

    while (!currentPagina1.isEmpty() && !currentPagina2.isEmpty()) {
      Tupla tupla1 = currentPagina1.tuplas[idxTupla1];
      Tupla tupla2 = currentPagina2.tuplas[idxTupla2];

      string valChave1 = getColunaValue(tupla1, idxChave1);
      string valChave2 = getColunaValue(tupla2, idxChave2);

      if (valChave1 < valChave2) {
        idxTupla1++;
        if (idxTupla1 >= currentPagina1.qtd_tuplas_ocup) {
          currentPagina1 = lerPaginaDeStream(file1, IOExecutados);
          idxTupla1 = 0;
        }
      } else if (valChave1 > valChave2) {
        idxTupla2++;
        if (idxTupla2 >= currentPagina2.qtd_tuplas_ocup) {
          currentPagina2 = lerPaginaDeStream(file2, IOExecutados);
          idxTupla2 = 0;
        }
      } else {
        // Chaves iguais, fazer a junção
        // Salvar a posição atual de file2 para "rebobinar" se houver duplicatas
        // em tabela1
        file2_pos_before_match = file2.tellg();
        // Precisamos salvar o estado da página e o índice da tupla
        Pagina savedPagina2 = currentPagina2;
        int savedIdxTupla2 = idxTupla2;

        vector<Tupla> blocoTabela2;
        // Coleta todas as tuplas da tabela2 que correspondem à chave atual
        int tempIdxTupla2 = idxTupla2;
        Pagina tempPagina2 = currentPagina2;
        while (true) {
          if (tempIdxTupla2 >= tempPagina2.qtd_tuplas_ocup) {
            tempPagina2 = lerPaginaDeStream(file2, IOExecutados);
            tempIdxTupla2 = 0;
            if (tempPagina2.isEmpty())
              break;
          }
          Tupla t2_temp = tempPagina2.tuplas[tempIdxTupla2];
          if (getColunaValue(t2_temp, idxChave2) == valChave1) {
            blocoTabela2.push_back(t2_temp);
            tempIdxTupla2++;
          } else {
            break;
          }
        }

        // Agora, para cada tupla da tabela1 com a mesma chave, junte com todas
        // do blocoTabela2
        string initial_valChave1 = valChave1;
        do {
          for (const auto &t2_match : blocoTabela2) {
            vector<string> tuplaJuntada = currentPagina1.tuplas[idxTupla1].cols;
            tuplaJuntada.insert(tuplaJuntada.end(), t2_match.cols.begin(),
                                t2_match.cols.end());
            tuplasGeradas.push_back(tuplaJuntada);
            tuplasGeradasCount++;
          }
          // Avança na tabela1 para a próxima tupla
          idxTupla1++;
          if (idxTupla1 >= currentPagina1.qtd_tuplas_ocup) {
            currentPagina1 = lerPaginaDeStream(file1, IOExecutados);
            idxTupla1 = 0;
          }
          if (!currentPagina1.isEmpty()) {
            tupla1 = currentPagina1.tuplas[idxTupla1];
            valChave1 = getColunaValue(tupla1, idxChave1);
          }
        } while (!currentPagina1.isEmpty() && valChave1 == initial_valChave1);

        // "Rebobinar" file2 para a posição antes do bloco de correspondência
        file2.clear(); // Limpa quaisquer flags de erro
        file2.seekg(file2_pos_before_match);
        currentPagina2 = savedPagina2; // Restaura a página
        idxTupla2 = savedIdxTupla2;    // Restaura o índice
      }
    }

    file1.close();
    file2.close();
    // A contagem de paginasGeradas para a junção será feita quando as tuplas
    // forem salvas em disco
    cout << "Merge Join concluído. Tuplas geradas: " << tuplasGeradasCount
         << endl;
  }

  // Implementação de executar
  void Operador::executar() {
    cout << "Executando Junção Sort-Merge..." << endl;
    string arq1Ordenado = tabela1.nomeArquivo + "_ordenado.csv";
    string arq2Ordenado = tabela2.nomeArquivo + "_ordenado.csv";

    // Carregar dados das tabelas se ainda não foram carregados
    // (Assumindo que carregarDados() preenche col_names e salva páginas em
    // disco)
    if (tabela1.pags.empty())
      tabela1.carregarDados();
    if (tabela2.pags.empty())
      tabela2.carregarDados();

    externalSort(tabela1, chave1, arq1Ordenado);
    externalSort(tabela2, chave2, arq2Ordenado);

    mergeJoin(arq1Ordenado, arq2Ordenado);

    // Salvar as tuplas geradas da junção em um arquivo final
    string nomeArquivoSaidaJuncao =
        "resultado_juncao_" +
        tabela1.nomeArquivo.substr(0, tabela1.nomeArquivo.find(".")) + "_" +
        tabela2.nomeArquivo.substr(0, tabela2.nomeArquivo.find(".")) + ".csv";
    salvarTuplasGeradas(nomeArquivoSaidaJuncao);

    cout << "Junção Sort-Merge concluída." << endl;
  }