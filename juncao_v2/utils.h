#pragma once

#include "pagina.h"
#include <fstream>
#include <string>
#include <vector>

// Função para ler uma página de um arquivo
Pagina lerPaginaDeArquivo(std::ifstream &arquivo, int &io_count);

// Função para escrever uma página em um arquivo
void escreverPaginaEmArquivo(std::ofstream &arquivo, const Pagina &pagina,
                             int &io_count);

// Função para ler todas as tuplas de um arquivo (para runs)
std::vector<Tupla> lerTodasTuplasDeArquivo(const std::string &nomeArquivo,
                                           int &io_count);

// Função para escrever um vetor de tuplas em um arquivo (para runs)
void escreverTuplasEmArquivo(const std::string &nomeArquivo,
                             const std::vector<Tupla> &tuplas, int &io_count);
