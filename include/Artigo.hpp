#ifndef ARTIGO_H
#define ARTIGO_H

#include <string>
#include <vector>

/**
 * @struct Artigo
 * @brief Representa um registro de dados de artigo científico com campos fixos para persistência em disco.
 */
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