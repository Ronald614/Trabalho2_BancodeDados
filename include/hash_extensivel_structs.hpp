#ifndef HASH_EXTENSIVEL_STRUCTS_HPP
#define HASH_EXTENSIVEL_STRUCTS_HPP

#include <cstddef>

struct HeaderHashExtensivel {

    int profundidade_global;
    size_t proximo_bloco_livre_dados;
    
};

const size_t PONTEIROS_POR_BLOCO_DIR = 512;

struct BlocoDiretorio {

    size_t ponteiros[PONTEIROS_POR_BLOCO_DIR];

};

#endif