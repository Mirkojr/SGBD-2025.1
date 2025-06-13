#ifndef TABELA_H
#define TABELA_H

#include <iostream>
#include <vector>
#include <string>
#include "Pagina.h" 

using namespace std;

class Tabela {
public:
    string nomeArquivo;
    vector<Pagina> pags;
    int qtd_pags;
    int qtd_cols;

    Tabela(const string& nomeArquivo);

    void carregarDados();
    void imprimir() const;
    void salvarPaginas(const string& nomeArquivo) const;
};

#endif // TABELA_H
