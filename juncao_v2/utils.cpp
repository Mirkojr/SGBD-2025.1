#include "utils.h"
#include <iostream>
#include <sstream>

// Função para ler um bloco de tuplas de um ifstream (simulando leitura de
// páginas)
std::vector<Tupla> lerBlocoDeTuplas(std::ifstream &arquivo, int num_paginas,
                                    int &io_count) {
  std::vector<Tupla> tuplasDoBloco;
  std::string linha;
  int tuplasLidas = 0;

  for (int p = 0; p < num_paginas; ++p) {
    bool paginaLida = false;
    for (int i = 0; i < MAX_TUPLES_PER_PAGE; ++i) {
      if (std::getline(arquivo, linha)) {
        if (!linha.empty()) {
          tuplasDoBloco.push_back(Tupla::fromString(linha));
          tuplasLidas++;
          paginaLida = true;
        }
      } else {
        break; // Fim do arquivo
      }
    }
    if (paginaLida) {
      io_count++; // Contabiliza leitura de página
    }
    if (!arquivo)
      break; // Se o arquivo terminou no meio de uma página
  }
  return tuplasDoBloco;
}

// Função auxiliar para obter o valor de uma coluna de uma tupla
std::string getColunaValue(const Tupla &tupla, int indiceColuna) {
  if (indiceColuna >= 0 &&
      static_cast<size_t>(indiceColuna) < tupla.cols.size()) {
    return tupla.cols[indiceColuna];
  }
  return ""; // Retorna string vazia se o índice for inválido
}

// Novas funções para I/O em nível de página
Pagina lerPaginaDeStream(std::ifstream &stream, int &io_count) {
  Pagina p;
  std::string line;
  bool pageRead = false;
  for (int i = 0; i < MAX_TUPLES_PER_PAGE; ++i) {
    if (std::getline(stream, line)) {
      if (!line.empty()) {
        p.adicionarTupla(Tupla::fromString(line));
        pageRead = true;
      }
    } else {
      break; // Fim do arquivo
    }
  }
  if (pageRead) {
    io_count++; // Contabiliza uma leitura de página
  }
  return p;
}

void escreverPaginaEmStream(std::ofstream &stream, const Pagina &pagina,
                            int &io_count) {
  for (int i = 0; i < pagina.qtd_tuplas_ocup; ++i) {
    stream << pagina.tuplas[i].toString() << std::endl;
  }
  if (pagina.qtd_tuplas_ocup >
      0) {      // Só contabiliza se algo foi realmente escrito
    io_count++; // Contabiliza uma escrita de página
  }
}
