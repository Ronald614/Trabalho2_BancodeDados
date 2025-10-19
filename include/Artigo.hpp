#ifndef ARTIGO_H
#define ARTIGO_H

#include <string>
#include <vector>

//Estrutura dos artigos no arquivo CSV
struct Artigo {

    int id;
    std::string titulo;
    int ano;
    std::string autores;
    int citacoes;
    std::string atualizacao;
    std::string snippet;
    
};

#endif