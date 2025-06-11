#ifndef OPERADOR_H
#define OPERADOR_H

#include <vector>
#include <string>
#include "Tabela.h"

using namespace std;

class Operador {
private:
    Tabela tabela1;
    Tabela tabela2;
    string chave1;
    string chave2;

    vector<vector<string>> tuplasGeradas; // resultado da junção

    int paginasGeradas = 0;
    int IOExecutados = 0;
    int tuplasGeradasCount = 0;

    // Métodos auxiliares
    void externalSort(Tabela& tabela, const string& chave, const string& nomeArquivoSaida);
    void mergeJoin(const string& arq1, const string& arq2);
    int getIndexColuna(const Tabela& tabela, const string& chave) const;

public:
    Operador(const Tabela& t1, const Tabela& t2, const string& chave1, const string& chave2);

    void executar(); // realiza a junção sort-merge
    void imprimirTuplasGeradas() const;
    void salvarTuplasGeradas(const string& nomeArquivo) const;

    int numPagsGeradas() const;
    int numIOExecutados() const;
    int numTuplasGeradas() const;
};

#endif
