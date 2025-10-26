#ifndef BLOCO_DE_DADOS_HPP
#define BLOCO_DE_DADOS_HPP

#include "Artigo.hpp"
#include <cstddef>

const size_t CAPACIDADE_BUCKET = 2;

struct BlocoDeDados {

    size_t contador_registros;
    long proximo_bloco_overflow;
    Artigo registros[CAPACIDADE_BUCKET];

};

inline size_t calcular_bloco_logico(size_t tamanho_bruto_bucket, size_t tamanho_bloco_so) {
    
    if (tamanho_bloco_so <= 0) {

        tamanho_bloco_so = 4096;

    }
    
    size_t num_blocos_so = (tamanho_bruto_bucket + tamanho_bloco_so - 1) / tamanho_bloco_so;
    
    return num_blocos_so * tamanho_bloco_so;
    
}

#endif

