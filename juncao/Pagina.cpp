#include "Pagina.h"
#include <iostream>

using namespace std;

void Pagina::adicionarTupla(const Tupla& tupla) {
    if (qtd_tuplas_ocup < 10) { // Verifica se há espaço na página
        tuplas[qtd_tuplas_ocup] = tupla; // Adiciona a tupla na próxima posição disponível
        qtd_tuplas_ocup++; // Incrementa o contador de tuplas ocupadas
    } else {
        cerr << "Erro: Página cheia, não é possível adicionar mais tuplas.\n";
    }
}

const Tupla* Pagina::getTuplas() const{
    return tuplas; // Retorna o array de tuplas
}
