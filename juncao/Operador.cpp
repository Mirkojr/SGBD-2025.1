#include "Operador.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;

Operador::Operador(const Tabela& t1, const Tabela& t2, const string& chave1, const string& chave2)
    : tabela1(t1), tabela2(t2), chave1(chave1), chave2(chave2),
      paginasGeradas(0), IOExecutados(0), tuplasGeradasCount(0) {executar();}

int Operador::getIndexColuna(const Tabela& tabela, const string& chave) const {
    // Busca o índice da coluna pelo cabeçalho do arquivo (primeira linha)
    ifstream arquivo(tabela.nomeArquivo);
    if (!arquivo.is_open()) return -1;

    string linha;
    if (getline(arquivo, linha)) {
        stringstream ss(linha);
        string coluna;
        int idx = 0;
        while (getline(ss, coluna, ',')) {
            if (coluna == chave) {
                return idx;
            }
            idx++;
        }
    }
    return -1;
}

#include <filesystem>

void Operador::externalSort(Tabela& tabela, const string& chave, const string& nomeArquivoSaida) {
    namespace fs = std::filesystem;
    const int PAGS_POR_RUN = 4;
    string tempDir = "tmp_extsort";
    fs::create_directory(tempDir);

    vector<string> tempFiles;
    int idx = getIndexColuna(tabela, chave);

    // 1. Leitura de runs de 4 páginas, ordenação e escrita em arquivos temporários
    for (size_t i = 0; i < tabela.pags.size(); i += PAGS_POR_RUN) {
        vector<Tupla> buffer;
        for (size_t j = i; j < i + PAGS_POR_RUN && j < tabela.pags.size(); ++j) {
            // Carrega a página do disco
            string nomePag = "pagina_" + (j + 1 < 10 ? "0" + to_string(j + 1) : to_string(j + 1)) + ".csv";
            ifstream arqPag(nomePag);
            string linha;
            getline(arqPag, linha); // Pula cabeçalho
            while (getline(arqPag, linha)) {
                stringstream ss(linha);
                string valor;
                Tupla tupla;
                while (getline(ss, valor, ',')) tupla.cols.push_back(valor);
                buffer.push_back(tupla);
            }
            arqPag.close();
            IOExecutados++; // Leitura da página
        }
        sort(buffer.begin(), buffer.end(), [idx](const Tupla& a, const Tupla& b) {
            return a.cols[idx] < b.cols[idx];
        });

        string tempFile = tempDir + "/run_" + to_string(i / PAGS_POR_RUN) + ".csv";
        ofstream arqTemp(tempFile);
        for (const auto& tupla : buffer) {
            for (size_t k = 0; k < tupla.cols.size(); ++k) {
                arqTemp << tupla.cols[k];
                if (k < tupla.cols.size() - 1) arqTemp << ",";
            }
            arqTemp << "\n";
        }
        arqTemp.close();
        IOExecutados++; // Escrita do run
        tempFiles.push_back(tempFile);
    }

    // 2. Merge dos arquivos temporários
    // Abre todos os arquivos temporários
    vector<ifstream> arqs;
    vector<string> linhas;
    vector<vector<string>> tuplas;
    for (const auto& f : tempFiles) {
        arqs.emplace_back(f);
        string linha;
        if (getline(arqs.back(), linha)) {
            stringstream ss(linha);
            string valor;
            vector<string> tupla;
            while (getline(ss, valor, ',')) tupla.push_back(valor);
            tuplas.push_back(tupla);
            linhas.push_back(linha);
        } else {
            tuplas.push_back({});
            linhas.push_back("");
        }
    }

    ofstream arqSaida(nomeArquivoSaida);
    while (true) {
        int menorIdx = -1;
        for (size_t i = 0; i < tuplas.size(); ++i) {
            if (!tuplas[i].empty()) {
                if (menorIdx == -1 || tuplas[i][idx] < tuplas[menorIdx][idx]) {
                    menorIdx = i;
                }
            }
        }
        if (menorIdx == -1) break; // Todos arquivos acabaram

        // Escreve a menor tupla
        for (size_t k = 0; k < tuplas[menorIdx].size(); ++k) {
            arqSaida << tuplas[menorIdx][k];
            if (k < tuplas[menorIdx].size() - 1) arqSaida << ",";
        }
        arqSaida << "\n";

        // Lê próxima tupla do arquivo correspondente
        string linha;
        if (getline(arqs[menorIdx], linha)) {
            stringstream ss(linha);
            string valor;
            vector<string> tupla;
            while (getline(ss, valor, ',')) tupla.push_back(valor);
            tuplas[menorIdx] = tupla;
        } else {
            tuplas[menorIdx].clear();
        }
    }
    arqSaida.close();
    IOExecutados++; // Escrita final

    // Limpa arquivos temporários
    for (const auto& f : tempFiles) fs::remove(f);
    fs::remove(tempDir);
}

void Operador::mergeJoin(const string& arq1, const string& arq2) {
    ifstream f1(arq1), f2(arq2);
    string linha1, linha2;
    vector<string> tupla1, tupla2;

    auto lerTupla = [](const string& linha) {
        vector<string> res;
        stringstream ss(linha);
        string valor;
        while (getline(ss, valor, ',')) res.push_back(valor);
        return res;
    };

    if (!getline(f1, linha1) || !getline(f2, linha2)) return;
    tupla1 = lerTupla(linha1);
    tupla2 = lerTupla(linha2);

    int idx1 = getIndexColuna(tabela1, chave1);
    int idx2 = getIndexColuna(tabela2, chave2);

    while (true) {
        if (tupla1[idx1] < tupla2[idx2]) {
            if (!getline(f1, linha1)) break;
            tupla1 = lerTupla(linha1);
        } else if (tupla1[idx1] > tupla2[idx2]) {
            if (!getline(f2, linha2)) break;
            tupla2 = lerTupla(linha2);
        } else {
            vector<vector<string>> buffer1, buffer2;
            string valorChave = tupla1[idx1];

            do {
                buffer1.push_back(tupla1);
                if (!getline(f1, linha1)) break;
                tupla1 = lerTupla(linha1);
            } while (tupla1[idx1] == valorChave);

            do {
                buffer2.push_back(tupla2);
                if (!getline(f2, linha2)) break;
                tupla2 = lerTupla(linha2);
            } while (tupla2[idx2] == valorChave);

            for (const auto& t1 : buffer1) {
                for (const auto& t2 : buffer2) {
                    vector<string> novaTupla = t1;
                    novaTupla.insert(novaTupla.end(), t2.begin(), t2.end());
                    tuplasGeradas.push_back(novaTupla);
                    tuplasGeradasCount++;
                }
            }
            paginasGeradas = (tuplasGeradasCount + 9) / 10;
            IOExecutados += 2; // Simula IO de leitura
            if (f1.eof() || f2.eof()) break;
        }
    }
    f1.close();
    f2.close();
}

void Operador::executar() {
    externalSort(tabela1, chave1, "tmp1.csv");
    externalSort(tabela2, chave2, "tmp2.csv");
    mergeJoin("tmp1.csv", "tmp2.csv");
}

void Operador::imprimirTuplasGeradas() const {
    for (const auto& tupla : tuplasGeradas) {
        for (const auto& valor : tupla) {
            cout << valor << " ";
        }
        cout << endl;
    }
}

void Operador::salvarTuplasGeradas(const string& nomeArquivo) const {
    ofstream arq(nomeArquivo);
    for (const auto& tupla : tuplasGeradas) {
        for (size_t i = 0; i < tupla.size(); ++i) {
            arq << tupla[i];
            if (i < tupla.size() - 1) arq << ",";
        }
        arq << "\n";
    }
    arq.close();
}

int Operador::numPagsGeradas() const {
    return paginasGeradas;
}

int Operador::numIOExecutados() const {
    return IOExecutados;
}

int Operador::numTuplasGeradas() const {
    return tuplasGeradasCount;
}