#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

using namespace std;

class Tabela {
public:

    string nomeArquivo;
    vector<string> colunas;
    vector<vector<string>> linhas;

    void carregarDados() {
        ifstream arquivo(nomeArquivo);
        if (!arquivo.is_open()) {
            cerr << "Erro ao abrir o arquivo: " << nomeArquivo << "\n";
            return;
        }

        string linha;
        // Lê o cabeçalho
        if (getline(arquivo, linha)) {
            stringstream ss(linha);
            string coluna;
            while (getline(ss, coluna, ',')) {
                colunas.push_back(coluna);
            }
        }

        // Lê os dados
        while (getline(arquivo, linha)) {
            stringstream ss(linha);
            string valor;
            vector<string> linhaDados;
            while (getline(ss, valor, ',')) {
                linhaDados.push_back(valor);
            }
            if (!linhaDados.empty())
                linhas.push_back(linhaDados);
        }
    }

public:
    Tabela(const string& nomeArquivo) {
        this->nomeArquivo = nomeArquivo;
    }

    void imprimir() const {
        for (const auto& col : colunas) {
            cout << col << "\t";
        }
        cout << "\n";
        for (const auto& linha : linhas) {
            for (const auto& valor : linha) {
                cout << valor << "\t";
            }
            cout << "\n";
        }
    }
};