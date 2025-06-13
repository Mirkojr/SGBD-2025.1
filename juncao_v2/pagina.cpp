#include "pagina.h"
#include <iostream>
#include <sstream>

using namespace std;

void Pagina::adicionarTupla(const Tupla &tupla) {
  if (qtd_tuplas_ocup < 10) { // Verifica se há espaço na página
    tuplas[qtd_tuplas_ocup] =
        tupla;         // Adiciona a tupla na próxima posição disponível
    qtd_tuplas_ocup++; // Incrementa o contador de tuplas ocupadas
  } else {
    cerr << "Erro: Página cheia, não é possível adicionar mais tuplas.\n";
  }
}

const Tupla *Pagina::getTuplas() const {
  return tuplas.data(); // Retorna um ponteiro para o array de tuplas
}

std::string Pagina::toString() const {
  std::string s = "";
  for (int i = 0; i < qtd_tuplas_ocup; ++i) {
    s += tuplas[i].toString();
    if (i < qtd_tuplas_ocup - 1) {
      s += "\n"; // Delimitador entre tuplas dentro da página
    }
  }
  return s;
}

Pagina Pagina::fromString(const std::string &s) {
  Pagina p;
  std::istringstream iss(s); // Usar istringstream para facilitar o parsing
  std::string line;
  while (std::getline(iss, line, '\n') &&
         p.qtd_tuplas_ocup < MAX_TUPLES_PER_PAGE) { // Delimitador entre tuplas
    p.adicionarTupla(Tupla::fromString(line));
  }
  return p;
}

