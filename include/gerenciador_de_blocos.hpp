#ifndef GERENCIADOR_DE_BLOCOS_HPP
#define GERENCIADOR_DE_BLOCOS_HPP

#include <string>
#include <cstddef>

/**
 * @class GerenciadorDeBlocos
 * 
 * @brief Gerencia um único arquivo de dados, mapeando-o em memória (mmap) e fornecendo acesso a blocos de tamanho fixo.
 *
 * Esta classe abstrai o I/O do arquivo. Os "dados" são lidos * ao se obter um ponteiro para uma região da memória mapeada.
 * Os "dados" são escritos quando essa região de memória é sincronizada com o disco.
 */

class GerenciadorDeBlocos {

    private:
        
        std::string caminho_arquivo;
        int arquivo_fd; // File descriptor do arquivo aberto.
        size_t tamanho_bloco;
        void* mapa_memoria;
        size_t tamanho_total_arquivo;

        long blocos_lidos;
        long blocos_escritos;

    public:

        GerenciadorDeBlocos(const std::string& caminho, size_t tamanho_bloco);

        ~GerenciadorDeBlocos();

        void* getPonteiroBloco(size_t id_bloco);
        void sincronizarBloco(size_t id_bloco);
        size_t alocarNovoBloco();

        long obterBlocosLidos() const { 
            
            return blocos_lidos; 
        
        }
        
        long obterBlocosEscritos() const { 
            
            return blocos_escritos; 
        
        }
        
        size_t obterTamanhoBloco() const { 
            
            return tamanho_bloco; 
        
        }
        
        size_t obterNumeroTotalBlocos() const {
            
            if (tamanho_bloco == 0) return 0;
            
            return tamanho_total_arquivo / tamanho_bloco;
        
        }

};

#endif