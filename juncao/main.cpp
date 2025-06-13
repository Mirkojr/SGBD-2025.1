#include <iostream>
#include <string>
#include "Tabela.h"
#include "Operador.h"

using namespace std;

int main() {
    Tabela vinho {"./tabelas_csv/vinho.csv"}; // cria estrutura necessaria para a tabela
    Tabela uva {"./tabelas_csv/uva.csv"};
    Tabela pais {"./tabelas_csv/pais.csv"};

    // vinho.carregarDados(); // le os dados do csv e add na estrutura da tabela
    uva.carregarDados();
    // pais.carregarDados();
    uva.imprimir(); // imprime os dados da tabela uva
    uva.salvarPaginas("paginas_uva");
    vinho.carregarDados();
    vinho.imprimir();
    vinho.salvarPaginas("paginas_vinhos");
    pais.carregarDados();
    pais.imprimir();
    pais.salvarPaginas("paginas_pais");

    Operador op {uva, vinho, "uva_vinho_id", "vinho_id"};
    op.salvarTuplasGeradas("juncao_uva_vinho.csv");
    // op.executar(); // Realiza a operacao desejada
    // op.imprimirTuplasGeradas(); // Imprime as tuplas geradas pela operacao
    // cout << "#Pags: " << op.numPagsGeradas(); // Retorna a quantidade de paginas geradas pela operacao
    // cout << "\n#IOss: " << op.numIOExecutados(); // Retorna a quantidade de IOs geradas pela operacao
    // cout << "\n#Tups: " << op.numTuplasGeradas(); // Retorna a quantidade de tuplas geradas pela operacao
    // op.salvarTuplasGeradas("juncao_uva_pais.csv"); // Retorna as tuplas geradas pela operacao e salva em um csv

    //IMPLEMENTE O OPERADOR E DEPOIS EXECUTE AQUI
    // Operador op {vinho, uva, "vinho_id", "uva_id"};
    //// significa: SELECT * FROM Vinho V, Uva U WHERE V.vinho_id = U.uva_id
    //// IMPORTANTE: isso eh so um exemplo, pode ser tabelas/colunas distintas.
    //// genericamente: Operador(tabela_1, tabela_2, col_tab_1, col_tab_2):
    //// significa: SELECT * FROM tabela_1, tabela_2 WHERE col_tab_1 = col_tab_2

    // op.executar(); // Realiza a operacao desejada
    // op.imprimirTuplasGeradas(); // Imprime as tuplas geradas pela operacao
    // cout << "#Pags: " << op.numPagsGeradas(); // Retorna a quantidade de paginas geradas pela operacao
    // cout << "\n#IOss: " << op.numIOExecutados(); // Retorna a quantidade de IOs geradas pela operacao
    // cout << "\n#Tups: " << op.numTuplasGeradas(); // Retorna a quantidade de tuplas geradas pela operacao

    // op.salvarTuplasGeradas("selecao_vinho_ano_colheita_1990.csv"); // Retorna as tuplas geradas pela operacao e salva em um csv

    return 0;
}