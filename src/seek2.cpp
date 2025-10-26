// Módulos C++
#include <iostream>
#include <string>
#include <stdexcept>
#include <chrono>
#include <optional>
#include <vector>
#include <cstring>
#include <set>
#include <fstream>

// Nossos módulos
#include "Log.hpp"
#include "Artigo.hpp"
#include "Parser.hpp"
#include "OSInfo.hpp"
#include "BlocoDeDados.hpp"
#include "config.hpp"
#include "GerenciadorArquivoDados.hpp"
#include "ArvoreBMais.hpp"

/**
 * @brief Programa seek2: Busca registros por Título usando o Índice Secundário B+Tree.
 *
 * Especificação: "seek2 <Titulo>: Programa que mostra os dados do registro que
 * possua o Título igual ao informado, se existir, pesquisando através do
 * arquivo de índice secundário, informando a quantidade de blocos lidos
 * para encontrá-lo no arquivo de índice e a quantidade total de blocos
 * do arquivo de índice secundário" 
 */

int main(int argc, char* argv[]) {

    log_init();

    //#################################################################
    // 1. Verificação de entrada
    //#################################################################

    if (argc != 2) {
    
        log_error("Uso incorreto.");
        log_error("Uso: " + std::string(argv[0]) + " \"<Titulo a ser buscado>\"");
        log_error("Exemplo Docker: docker compose run --rm seek2 \"Um Titulo Exato\"");

        return 1;
    
    }

    const std::string titulo_busca = argv[1];
    if (titulo_busca.length() > 299) {

        log_error("Erro: Título muito longo. Máximo de 299 caracteres.");
        
        return 1;
    
    }

    //#################################################################
    // 2. Definição dos caminhos e início dos logs
    //#################################################################
    const std::string dataDir = "/data/db";
    const std::string diretorio_hash_dados = dataDir + "/artigos.dat";
    const std::string btreeTituloPath = dataDir + "/btree_titulo.idx";

    log_info("--- Iniciando Busca (seek2) ---");
    log_info("Buscando Título: \"" + titulo_busca + "\"");
    log_info("Usando Índice Secundário (B+Tree): " + btreeTituloPath);
    log_info("Lendo de Arquivo de Dados (Hash): " + diretorio_hash_dados);

    //#################################################################
    // 3. Configuração dos Gerenciadores
    //#################################################################

    const std::string metaDir = dataDir + "/db.meta";
    size_t TAMANHO_BLOCO_LOGICO_DADOS = 0;
    size_t TAMANHO_BLOCO_BTREE = 0;

    std::ifstream meta_info(metaDir, std::ios::binary);
    
    if (!meta_info.is_open()) {
    
        log_error("Falha fatal ao ler arquivo de metadados: " + metaDir);
    
        log_error("Execute o 'upload' primeiro para criar os arquivos de banco de dados.");

        return 1;
    
    }

    meta_info.read(reinterpret_cast<char*>(&TAMANHO_BLOCO_LOGICO_DADOS), sizeof(size_t));
    meta_info.read(reinterpret_cast<char*>(&TAMANHO_BLOCO_BTREE), sizeof(size_t));

    meta_info.close();

    if (TAMANHO_BLOCO_LOGICO_DADOS == 0 || TAMANHO_BLOCO_BTREE == 0) {
        
        log_error("Arquivo de metadados inválido ou corrompido: " + metaDir);
    
        return 1;
    
    }

    log_debug("Tamanho do Bloco de Dados lido de .meta: " + std::to_string(TAMANHO_BLOCO_LOGICO_DADOS));
    log_debug("Tamanho do Bloco de Índice lido de .meta: " + std::to_string(TAMANHO_BLOCO_BTREE));

    std::vector<Artigo> resultados;
    long blocos_lidos_indice = 0;
    long total_blocos_indice = 0;
    long duration_ms = 0;

    //#################################################################
    // 4. Execução da Busca
    //#################################################################

    try {
        
        BPlusTree<ChaveTitulo> btree_titulo(btreeTituloPath, TAMANHO_BLOCO_BTREE);

        GerenciadorArquivoDados gerenciador_dados_hash(diretorio_hash_dados, TAMANHO_BLOCO_LOGICO_DADOS);
        
        ChaveTitulo chave_busca;
        strncpy(chave_busca.titulo, titulo_busca.c_str(), 300);
        chave_busca.titulo[299] = '\0';

        auto startTime = std::chrono::high_resolution_clock::now();

        // 1. Busca no Índice B+Tree. Retorna um VETOR de IDs de bucket.
        std::vector<long> ids_buckets_brutos = btree_titulo.search(chave_busca);

        // 2. Remove duplicatas usando um std::set. (ex: [1, 1] vira {1})
        std::set<long> ids_buckets_unicos(ids_buckets_brutos.begin(), ids_buckets_brutos.end());

        // 2. Para cada bucket ID encontrado, busca no arquivo de dados
        for (long id_bucket : ids_buckets_unicos) {
            
            BlocoDeDados* bucket = static_cast<BlocoDeDados*>(gerenciador_dados_hash.getPonteiroBloco(id_bucket));

            // 3. Varre o bucket para encontrar o(s) registro(s) com o título exato
            for (size_t i = 0; i < bucket->contador_registros; ++i) {

                // Compara o título do registro com a busca
                if (std::strcmp(bucket->registros[i].titulo, titulo_busca.c_str()) == 0) {
                
                    resultados.push_back(bucket->registros[i]);
                
                }
            
            }
        
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        // Estatísticas
        blocos_lidos_indice = btree_titulo.getIndexBlocosLidos();
        total_blocos_indice = btree_titulo.getIndexTotalBlocos();
        
    }
    
    catch (const std::exception& e) {

        log_error("Erro Fatal durante a busca: " + std::string(e.what()));
        
        return 1;
    
    }

    //#################################################################
    // 5. Relatório de Resultados e Estatísticas
    //#################################################################

    if (!resultados.empty()) {
    
        log_info("--- " + std::to_string(resultados.size()) + " Registro(s) Encontrado(s) ---");
    
        for (const auto& artigo : resultados) {
    
            printArtigo(artigo);
    
        }
    
    }
    
    else {
    
        log_info("--- Nenhum registro com o Título \"" + titulo_busca + "\" foi encontrado. ---");
    
    }

    log_info("\n--- Estatísticas da Operação (seek2) ---");
    log_info("Tempo total de execução: " + std::to_string(duration_ms) + " ms");
    log_info("Arquivo de Índice Secundário: " + btreeTituloPath);
    log_info("  - Blocos lidos (Índice): " + std::to_string(blocos_lidos_indice));
    log_info("  - Total de blocos (Índice): " + std::to_string(total_blocos_indice));

    return 0;

}