#include "tupla.h"
#include <algorithm> // For std::min
#include <iostream>
#include <sstream>

using namespace std;

Tupla::Tupla(size_t qtd_cols) : cols(qtd_cols) {}
Tupla::Tupla(const vector<string> &valores) : cols(valores) {}

void Tupla::imprimir() const {
  for (size_t i = 0; i < cols.size(); ++i) {
    cout << cols[i];
    if (i != cols.size() - 1)
      cout << " ";
  }
  cout << endl;
}

bool Tupla::operator<(const Tupla &other) const {
  size_t min_cols = std::min(cols.size(), other.cols.size());
  for (size_t i = 0; i < min_cols; ++i) {
    if (cols[i] < other.cols[i])
      return true;
    if (cols[i] > other.cols[i])
      return false;
  }
  return cols.size() < other.cols.size();
}

bool Tupla::operator==(const Tupla &other) const { return cols == other.cols; }

std::string Tupla::toString() const {
  std::string s = "";
  for (size_t i = 0; i < cols.size(); ++i) {
    s += cols[i];
    if (i < cols.size() - 1) {
      s += ";"; // Delimitador entre colunas
    }
  }
  return s;
}

Tupla Tupla::fromString(const std::string &s) {
  std::vector<std::string> parsed_cols;
  std::string current_col;
  std::istringstream iss(s); // Usar istringstream para facilitar o parsing
  while (getline(iss, current_col, ';')) { // Delimitador entre colunas
    parsed_cols.push_back(current_col);
  }
  return Tupla(parsed_cols);
}
