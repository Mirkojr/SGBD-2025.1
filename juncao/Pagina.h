#pragma once

#include "Tupla.h"

class Pagina {
public:
    Tupla tuplas[10];
    int qtd_tuplas_ocup;

    Pagina() : qtd_tuplas_ocup(0) {}

    void adicionarTupla(const Tupla& tupla);
    const Tupla* getTuplas() const;
};