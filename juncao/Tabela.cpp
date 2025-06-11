#include "Tabela.h"
#include "Pagina.h"
#include "Tupla.h"

#include <fstream>
#include <sstream>

Tabela::Tabela(const string& nomeArquivo) {
    this->nomeArquivo = nomeArquivo;
    this->qtd_pags = 0;
    this->qtd_cols = 0;
}

void Tabela::carregarDados() {
    cout << "Carregando dados da tabela: " << nomeArquivo << endl;

    ifstream arquivo(nomeArquivo);
    if (!arquivo.is_open()) {
        cerr << "Erro ao abrir o arquivo: " << nomeArquivo << endl;
        return;
    }

    cout << "Arquivo aberto com sucesso!" << endl; 

    string linha;
    // Lê o cabeçalho
    vector<string> colunas;
    if (getline(arquivo, linha)) {
        // cout << "Lendo cabeçalho: " << linha << endl;

        stringstream ss(linha);
        string coluna;
        while (getline(ss, coluna, ',')) {
            colunas.push_back(coluna);
        }

        // cout << "Colunas encontradas: ";
        // for (const auto& col : colunas) {
        //     cout << col << " " << endl;
        // }

        qtd_cols = colunas.size();
    }

    // Lê os dados e armazena em páginas
    Pagina paginaAtual;
    int tam_pagina = 10; // Exemplo: 10 linhas por página
    int linhas_na_pagina = 0;

    while (getline(arquivo, linha)) {
        // cout << "Lendo linha: " << linha << endl;
        stringstream ss(linha);
        string valor;
        vector<string> linhaDados;
        while (getline(ss, valor, ',')) {

            linhaDados.push_back(valor);
        }
        if (!linhaDados.empty()) {
            Tupla tupla(linhaDados); // Supondo que Tupla tem um construtor que aceita vector<string>
            paginaAtual.adicionarTupla(tupla);
            linhas_na_pagina++;
        }
        if (linhas_na_pagina == tam_pagina) {

            pags.push_back(paginaAtual);
            paginaAtual = Pagina();
            linhas_na_pagina = 0;
            qtd_pags++;
        }
    }
    if (linhas_na_pagina > 0) {
        pags.push_back(paginaAtual);
        qtd_pags++;
    }
}

void Tabela::imprimir() const {
    cout << "Tabela: " << nomeArquivo << "\n";
    cout << "Quantidade de colunas: " << qtd_cols << "\n";
    cout << "Quantidade de páginas: " << qtd_pags << "\n";

    for (const auto& pagina : pags) {
        for (int i = 0; i < pagina.qtd_tuplas_ocup; ++i) {
            pagina.tuplas[i].imprimir();
        }
    }
}