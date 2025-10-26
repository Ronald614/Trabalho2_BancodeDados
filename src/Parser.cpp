#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <algorithm>

#include "Parser.hpp"

void printArtigo(const Artigo& artigo) {

    std::cout << "-------------------------------------\n";
    std::cout << "---> Imprimindo artigo.\n";

    std::cout << "ID: " << artigo.id << "\n";
    std::cout << "Ano: " << artigo.ano << "\n";
    std::cout << "Citacoes: " << artigo.citacoes << "\n";

    std::cout << "Titulo: " << (artigo.titulo[0] == '\0' ? "[VAZIO]" : artigo.titulo) << "\n";
    std::cout << "Autores: " << (artigo.autores[0] == '\0' ? "[VAZIO]" : artigo.autores) << "\n";
    std::cout << "Atualizacao: " << (artigo.atualizacao[0] == '\0' ? "[VAZIO]" : artigo.atualizacao) << "\n";
    std::cout << "Snippet: " << (artigo.snippet[0] == '\0' ? "[VAZIO]" : artigo.snippet) << "\n";

    std::cout << "-------------------------------------\n";

}

void copiarStringSeguro(char* destino, const std::string& origem, size_t tamanhoDestino) {
    
    // Garante que o número máximo de caracteres copiado seja (tamanhoDestino - 1) para reservar 1 byte para o terminador nulo ('\0').
    std::strncpy(destino, origem.c_str(), tamanhoDestino - 1);
    
    // Garante que a string de destino seja terminada em nulo, mesmo que a origem tenha sido maior que (tamanhoDestino - 1).
    destino[tamanhoDestino - 1] = '\0';

}

int stringParaIntSeguro(const std::string& texto) {
    
    if (texto.empty()) {
        
        return 0;
        
    }
    
    try {
        
        return std::stoi(texto);
        
    }
    
    catch (const std::exception&) {

        return 0;

    }

}

std::string limpaCampo(const std::string& campo_bruto) {
    
    if (campo_bruto.empty()) {
        
        return "";
        
    }
    
    if (campo_bruto == "NULL") {
        
        return "";
        
    }

    if (campo_bruto.length() >= 2 && campo_bruto.front() == '"' && campo_bruto.back() == '"') {
        
        return campo_bruto.substr(1, campo_bruto.length() - 2);
    
    }
    
    return campo_bruto;

}

std::vector<std::string> divideCSVLinha(const std::string& linha) {
    
    std::vector<std::string> campos;
    std::string campo_atual;
    
    bool entre_aspas = false;
    
    int contador_ponto_virgula = 0;

    for (size_t i = 0; i < linha.length(); ++i) {
        
        char caractere = linha[i];

        if (caractere == '"') {

            entre_aspas = !entre_aspas;
            campo_atual += caractere;
        
        }
        
        else if (caractere == ';' && !entre_aspas) {
            
            campos.push_back(campo_atual);
            campo_atual = "";
            contador_ponto_virgula++;

            if (contador_ponto_virgula == 6) {
                
                campos.push_back(linha.substr(i + 1));
            
                return campos;
            
            }

        }
        
        else {

            campo_atual += caractere;

        }

    }
    
    campos.push_back(campo_atual);

    return campos;

}

bool parseCSVLinha(const std::string& linha, Artigo& artigo_saida) {
    
    // Passo 1: Usar o split de máquina de estados com limite.
    std::vector<std::string> campos = divideCSVLinha(linha);

    // Passo 2: Validar o resultado.
    // ---> Se a linha tiver 6 ou mais delimitadores reais, a função split sempre retornará 7 campos.
    // ---> Se tiver menos de 6, ela retornará menos de 7.

    if (campos.size() != 7) {
        
        std::cout << "[Parser] Linha ignorada (mal formatada, campos != 7): " << linha << std::endl;
        
        return false;
    
    }

    // Passo 3: Limpar os campos e preencher o struct
    try {

        std::string id_limpo = limpaCampo(campos[0]);
        
        artigo_saida.id = stringParaIntSeguro(id_limpo);
        
        if (artigo_saida.id == 0) {
            
            std::cout << "[Parser] Linha ignorada (ID invalido ou zero): " << linha << std::endl;
            
            return false;
        
        }

        copiarStringSeguro(artigo_saida.titulo, limpaCampo(campos[1]), 300);
        
        artigo_saida.ano = stringParaIntSeguro(limpaCampo(campos[2]));

        copiarStringSeguro(artigo_saida.autores, limpaCampo(campos[3]), 150);
        
        artigo_saida.citacoes = stringParaIntSeguro(limpaCampo(campos[4]));
        
        copiarStringSeguro(artigo_saida.atualizacao, limpaCampo(campos[5]), 20);
        
        copiarStringSeguro(artigo_saida.snippet, limpaCampo(campos[6]), 1024);

    }
    
    catch (const std::exception& e) {
        
        std::cerr << "[Parser] Erro inesperado ao atribuir campos: " << e.what() << std::endl;
        
        return false;
    
    }

    return true;

}