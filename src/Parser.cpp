#include "Parser.hpp"
#include <sstream>
#include <string>
#include <vector>
#include <iostream>

std::string remove_aspas (const std::string& str) {
    
    if (str.length() >= 2 && str.front() == '"' && str.back() == '"') {
    
        return str.substr(1, str.length() - 2);
    
    }
    
    return str;

}

Artigo parseCSVLine(const std::string& line) {

    std::stringstream ss(line);
    std::string campo;
    std::vector<std::string> lista_de_campos;

    while (std::getline(ss, campo, ';')) {

        lista_de_campos.push_back(campo);
    
    }

    Artigo artigo;

    try {

        artigo.id = !lista_de_campos[0].empty() ? std::stoi(remove_aspas(lista_de_campos[0])) : 0;
        
        artigo.titulo = remove_aspas(lista_de_campos[1]);
        
        artigo.ano = !lista_de_campos[2].empty() ? std::stoi(remove_aspas(lista_de_campos[2])) : 0;
        
        artigo.autores = remove_aspas(lista_de_campos[3]);
        
        artigo.citacoes = !lista_de_campos[4].empty() ? std::stoi(remove_aspas(lista_de_campos[4])) : 0;
        
        artigo.atualizacao = remove_aspas(lista_de_campos[5]);
        
        artigo.snippet = remove_aspas(lista_de_campos[6]);

    }
    
    catch (const std::invalid_argument& e) {
        
        std::cerr << "Erro de conversão de tipo na linha: " << line << std::endl;

        artigo.id = -1; 
    
    }
    
    catch (const std::out_of_range& e) {
    
        std::cerr << "Erro de valor fora do range na linha: " << line << std::endl;
        
        artigo.id = -1;
    
    }

    return artigo;

}