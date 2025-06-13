#include "Tabela.h"
#include "Pagina.h"
#include "Tupla.h"

#include <fstream>
#include <sstream>
#include <iomanip>

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

        stringstream ss(linha);
        string coluna;
        while (getline(ss, coluna, ',')) {
            colunas.push_back(coluna);
        }

        qtd_cols = colunas.size();
    }

    // Lê os dados e armazena em páginas
    Pagina paginaAtual;
    int tam_pagina = 10; // Exemplo: 10 linhas por página
    int linhas_na_pagina = 0;

    while (getline(arquivo, linha)) {
        stringstream ss(linha);
        string valor;
        vector<string> linhaDados;
        while (getline(ss, valor, ',')) {

            linhaDados.push_back(valor);
        }
        if (!linhaDados.empty()) {
            Tupla tupla(linhaDados); 
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

void Tabela::salvarPaginas(const string& pasta) const {
    // Cria a pasta se não existir
    string comando = "mkdir -p " + pasta;
    if (system(comando.c_str()) != 0) {
        cerr << "Erro ao criar a pasta: " << pasta << endl;
        return;
    }

    for (size_t i = 0; i < pags.size(); ++i) {
        // Nome do arquivo da página
        ostringstream nomeArquivoPagina;
        nomeArquivoPagina << pasta << "/pagina_" << setfill('0') << setw(2) << (i + 1) << ".csv";
        ofstream arquivoPagina(nomeArquivoPagina.str());
        if (!arquivoPagina.is_open()) {
            cerr << "Erro ao criar arquivo: " << nomeArquivoPagina.str() << endl;
            continue;
        }

        // Escreve as tuplas da página
        const Pagina& pagina = pags[i];
        for (int j = 0; j < pagina.qtd_tuplas_ocup; ++j) {
            const Tupla& tupla = pagina.tuplas[j];
            for (size_t k = 0; k < tupla.cols.size(); ++k) {
                arquivoPagina << tupla.cols[k];
                if (k < tupla.cols.size() - 1)
                    arquivoPagina << ",";
            }
            arquivoPagina << "\n";
        }
        arquivoPagina.close();
    }
}