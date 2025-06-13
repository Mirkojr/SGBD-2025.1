#include "tabela.h"
#include "pagina.h"
#include "tupla.h"

#include <fstream>
#include <sstream>

using namespace std;

Tabela::Tabela(const string &nomeArquivo) {
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
    col_names = colunas; // Armazena os nomes das colunas
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
      Tupla tupla(linhaDados); // Supondo que Tupla tem um construtor que aceita
                               // vector<string>
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
  salvarPaginasEmDisco(); // Salva as páginas em disco após carregar
}

void Tabela::imprimir() const {
  cout << "Tabela: " << nomeArquivo << "\n";
  cout << "Quantidade de colunas: " << qtd_cols << "\n";
  cout << "Quantidade de páginas: " << qtd_pags << "\n";

  for (const auto &pagina : pags) {
    for (int i = 0; i < pagina.qtd_tuplas_ocup; ++i) {
      pagina.tuplas[i].imprimir();
    }
  }
}

void Tabela::salvarPaginasEmDisco() const {
  string dirName = "data/" + nomeArquivo.substr(0, nomeArquivo.find("."));
  string command = "mkdir -p " + dirName; // Criar diretório se não existir
  system(command.c_str());

  for (size_t i = 0; i < pags.size(); ++i) {
    string pageFileName = dirName + "/pagina_" + to_string(i) + ".txt";
    ofstream outFile(pageFileName);
    if (!outFile.is_open()) {
      cerr << "Erro ao criar arquivo de página: " << pageFileName << endl;
      continue;
    }
    outFile << pags[i].toString();
    outFile.close();
  }
  cout << "Páginas da tabela " << nomeArquivo << " salvas em " << dirName
       << endl;
}
