#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <algorithm>
#include <cctype>

using namespace std;

// Estrutura para armazenar informações de um dado
struct DataInfo {
    int ts_read;    // Timestamp da última leitura
    int ts_write;   // Timestamp da última escrita
};

// Função para remover espaços e caracteres especiais
string clean_token(string token) {
    token.erase(remove_if(token.begin(), token.end(), ::isspace), token.end());
    token.erase(remove(token.begin(), token.end(), ';'), token.end());
    return token;
}

// Função principal
int main() {
    ifstream input("in.txt");
    ofstream output("out.txt");
    
    if (!input.is_open() || !output.is_open()) {
        cerr << "Erro ao abrir arquivos!" << endl;
        return 1;
    }

    // Ler objetos de dados
    string line;
    getline(input, line);
    stringstream ss_data(line);
    vector<string> data_objects;
    string token;
    
    while (getline(ss_data, token, ',')) {
        token = clean_token(token);
        if (!token.empty()) {
            data_objects.push_back(token);
            cout << "Dado: " << token << endl;
            // Criar arquivo do dado
            ofstream data_file(token + ".txt", ios::trunc);
            data_file.close();
        }
    }

    // Ler transações
    getline(input, line);
    stringstream ss_trans(line);
    vector<string> transactions;
    
    while (getline(ss_trans, token, ',')) {
        token = clean_token(token);
        if (!token.empty()) {
            cout << "Transação: " << token << endl;
            transactions.push_back(token);
        }
    }

    // Ler timestamps
    getline(input, line);
    stringstream ss_ts(line);
    vector<int> timestamps;
    
    while (getline(ss_ts, token, ',')) {
        token = clean_token(token);
        if (!token.empty()) {
            cout << "Timestamp: " << token << endl;
            timestamps.push_back(stoi(token));
        }
    }

    // Mapear transações para timestamps
    map<string, int> ts_map;
    for (size_t i = 0; i < transactions.size(); i++) {
        string t_id = transactions[i].substr(1); // Remove 't'
        cout << "Mapeando transação " << t_id << " para timestamp " << timestamps[i] << endl;
        ts_map[t_id] = timestamps[i];
    }

    // Processar escalonamentos
    while (getline(input, line)) {
        if (line.empty()) continue;
        
        // Identificar escalonamento
        size_t pos = line.find('-');
        string schedule_id = line.substr(0, pos);
        string ops_str = line.substr(pos + 1);
        
        // Tokenizar operações
        stringstream ss_ops(ops_str);
        vector<string> operations;
        while (ss_ops >> token) { // Lê até o próximo espaço
            operations.push_back(token);
        }

        // Exibir informações do escalonamento no console
        cout << "Operações do escalonamento " << schedule_id << ": ";
        for (const string& op : operations) {
            cout << op << " ";
        }
        cout << endl;

        // Inicializar estruturas
        map<string, DataInfo> data_table;
        for (const auto& obj : data_objects) {
            data_table[obj] = {-1, -1}; // TS inicial = -1
        }
        
        set<string> aborted_trans;
        int moment = 0;
        bool has_rollback = false;
        int rollback_moment = -1;

        // Processar cada operação
        for (const string& op : operations) {
            char op_type = op[0];
            string t_id;
            string data_item;
            
            // Extrair informações da operação
            if (op_type == 'r' || op_type == 'w') {
                size_t start = 1;
                size_t end = op.find('(');
                
                if (end != string::npos) {
                    t_id = op.substr(start, end - start);
                    start = end + 1;
                    end = op.find(')');
                    data_item = op.substr(start, end - start);
                }
            } else if (op_type == 'c') { // Commit
                t_id = op.substr(1);
                // Reinicializa os timestamps dos dados acessados por esta transação
               for (auto& [data_item, info] : data_table) {
                    info.ts_read = -1;
                    info.ts_write = -1;
                }
                moment++;
                continue;
            } else {
                continue; // Operação desconhecida
            }

            // Verificar se transação está abortada
            if (aborted_trans.find(t_id) != aborted_trans.end()) {
                moment++;
                continue;
            }

            // Registrar operação no arquivo do dado
            ofstream data_file(data_item + ".txt", ios::app);
            data_file << schedule_id << " " 
                     << (op_type == 'r' ? "R" : "W") << " " 
                     << moment << endl;
            data_file.close();

            // Processar operação
            if (op_type == 'r') { // Leitura
                if (ts_map[t_id] < data_table[data_item].ts_write) {
                    aborted_trans.insert(t_id);
                    if (!has_rollback) {
                        has_rollback = true;
                        rollback_moment = moment;
                    }
                } else {
                    data_table[data_item].ts_read = max(
                        data_table[data_item].ts_read, 
                        ts_map[t_id]
                    );
                }
            } else if (op_type == 'w') { // Escrita
                if (ts_map[t_id] < data_table[data_item].ts_read || 
                    ts_map[t_id] < data_table[data_item].ts_write) {
                    aborted_trans.insert(t_id);
                    if (!has_rollback) {
                        has_rollback = true;
                        rollback_moment = moment;
                    }
                } else {
                    data_table[data_item].ts_write = ts_map[t_id];
                }
            }
            
            moment++;
        }

        // Registrar resultado do escalonamento
        if (has_rollback) {
            output << schedule_id << "-ROLLBACK-" << rollback_moment << endl;
        } else {
            output << schedule_id << "-OK" << endl;
        }
    }

    input.close();
    output.close();
    return 0;
}