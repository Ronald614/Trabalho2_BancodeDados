#ifndef ARTIGO_H
#define ARTIGO_H

#include <string>
#include <vector>

//Estrutura dos artigos no arquivo CSV
struct Artigo {

    int id;
    char titulo[300];
    int ano;
    char autores[150];
    int citacoes;
    char atualizacao[20];
    char snippet[1024];
    
};

#endif