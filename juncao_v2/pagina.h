#pragma once

#include "tupla.h"
#include <array>

const int MAX_TUPLES_PER_PAGE = 10;

class Pagina {
public:
  std::array<Tupla, MAX_TUPLES_PER_PAGE> tuplas;
  int qtd_tuplas_ocup;

  Pagina() : qtd_tuplas_ocup(0) {}

  void adicionarTupla(const Tupla &tupla);
  const Tupla *getTuplas() const;
  std::string toString() const;
  static Pagina fromString(const std::string &s);
  bool isFull() const { return qtd_tuplas_ocup == MAX_TUPLES_PER_PAGE; }
  bool isEmpty() const { return qtd_tuplas_ocup == 0; }
};
