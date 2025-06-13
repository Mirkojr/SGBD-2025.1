#include <string>
#include <vector>
#pragma once

using namespace std;
class Tupla {
public:
  vector<string> cols;

  Tupla() = default;
  Tupla(size_t qtd_cols);
  Tupla(const vector<string> &valores);
  void imprimir() const;
  bool operator<(const Tupla &other) const;
  bool operator==(const Tupla &other) const;
  std::string toString() const;
  static Tupla fromString(const std::string &s);
};
