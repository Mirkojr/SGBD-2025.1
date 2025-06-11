#include <iostream>
#include <string>
#include "tabela.cpp"
// VOCE DEVE INCLUIR SEUS HEADERS AQUI, CASO NECESSARIO!!!


using namespace std;

int main() {
    Tabela vinho {"vinho.csv"}; // cria estrutura necessaria para a tabela
    Tabela uva {"uva.csv"};
    Tabela pais {"pais.csv"};

    vinho.carregarDados(); // le os dados do csv e add na estrutura da tabela, caso necessario
    uva.carregarDados();
    pais.carregarDados();
    vinho.imprimir(); // imprime os dados da tabela vinho
    
    //IMPLEMENTE O OPERADOR E DEPOIS EXECUTE AQUI
    // Operador op {vinho, uva, "vinho_id", "uva_id"};
    //// significa: SELECT * FROM Vinho V, Uva U WHERE V.vinho_id = U.uva_id
    //// IMPORTANTE: isso eh so um exemplo, pode ser tabelas/colunas distintas.
    //// genericamente: Operador(tabela_1, tabela_2, col_tab_1, col_tab_2):
    //// significa: SELECT * FROM tabela_1, tabela_2 WHERE col_tab_1 = col_tab_2

    //op.executar(); // Realiza a operacao desejada

    // cout << "#Pags: " << op.numPagsGeradas(); // Retorna a quantidade de paginas geradas pela operacao
    // cout << "\n#IOss: " << op.numIOExecutados(); // Retorna a quantidade de IOs geradas pela operacao
    // cout << "\n#Tups: " << op.numTuplasGeradas(); // Retorna a quantidade de tuplas geradas pela operacao

    // op.salvarTuplasGeradas("selecao_vinho_ano_colheita_1990.csv"); // Retorna as tuplas geradas pela operacao e salva em um csv

    return 0;
}