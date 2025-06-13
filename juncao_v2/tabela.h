#ifndef TABELA_H
#define TABELA_H

#include "pagina.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

class Tabela {
public:
  string nomeArquivo;
  vector<Pagina> pags;
  int qtd_pags;
  int qtd_cols;
  vector<string> col_names; // Adicionado para armazenar nomes das colunas

  Tabela(const string &nomeArquivo);

  void carregarDados();
  void imprimir() const;
  void salvarPaginasEmDisco()
      const; // Novo método para salvar páginas individualmente
};

#endif // TABELA_H