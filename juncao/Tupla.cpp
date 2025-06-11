#include "Tupla.h"
#include <iostream>

using namespace std;

Tupla::Tupla(size_t qtd_cols) : cols(qtd_cols) {}
Tupla::Tupla(const vector<string>& valores) : cols(valores) {}

void Tupla::imprimir() const {
    for (size_t i = 0; i < cols.size(); ++i) {
        cout << cols[i];
        if (i != cols.size() - 1) cout << " "; 
    }
    cout << endl;
}