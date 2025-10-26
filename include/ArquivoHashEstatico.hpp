#ifndef HASH_ESTATICO_HPP
#define HASH_ESTATICO_HPP

#include <optional>
#include <cstddef>

#include "GerenciadorArquivoDados.hpp"
#include "BlocoDeDados.hpp"
#include "Artigo.hpp"

/**
 * @class ArquivoHashEstatico
 * @brief Implementa uma estrutura de Hashing Estático com resolução de colisão
 * por encadeamento de blocos de overflow.
 */
class ArquivoHashEstatico {

    private:
        GerenciadorArquivoDados& gerenciador_dados;
        const int NUM_BUCKETS_PRIMARIOS;

        /**
         * @brief Calcula o ID do bucket primário (0 a N-1) para um dado ID, usando módulo.
         * @param id O ID do artigo.
         * @return O ID do bucket primário.
         */
        int hash(int id);

        /**
         * @brief Aloca e inicializa um novo bloco de dados (bucket) no arquivo, definindo o ponteiro de overflow para -1.
         * @return O ID (índice) do bloco recém-alocado.
         */
        size_t alocarNovoBucket();

    public:
        /**
         * @brief Construtor.
         * @param gm_dados Gerenciador do arquivo de dados.
         * @param num_buckets O número 'N' de buckets primários.
         */
        ArquivoHashEstatico(GerenciadorArquivoDados& gm_dados, int num_buckets);
        
        /**
         * @brief Inicializa a tabela hash, alocando e zerando os N buckets primários se o arquivo estiver vazio.
         */
        void inicializar();
        
        /**
         * @brief Insere um artigo, tratando colisões por encadeamento de overflow e alocando novos blocos se necessário.
         * @param a O Artigo a ser inserido.
         * @return O ID do bloco onde o artigo foi inserido.
         */
        size_t inserir(const Artigo& a); 
        
        /**
         * @brief Busca um artigo pelo ID, percorrendo o bucket primário e sua cadeia de overflow.
         * @param id O ID a ser buscado.
         * @return O Artigo encontrado (ou vazio, se não encontrado).
         */
        std::optional<Artigo> buscar(int id);
        
};

#endif