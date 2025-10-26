// Módulos C++
#include <iostream>
#include <string>
#include <stdexcept>
#include <chrono>
#include <optional>
#include <iomanip>
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
 * @brief Programa seek1: Busca um registro pelo ID usando o Índice Primário B+Tree.
 *
 * Especificação: "seek1 <ID>: Programa que devolve o registro com ID igual ao
 * informado, se existir, pesquisando através do arquivo de índice primário,
 * mostrando todos os campos, a quantidade de blocos lidos para encontrá-lo
 * no arquivo de índice e a quantidade total de blocos do arquivo de índice primário"
 */

int main(int argc, char* argv[]) {

    log_init();

    //#################################################################
    // 1. Verificação de entrada
    //#################################################################
    if (argc != 2) {

        log_error("Uso incorreto.");
        log_error("Uso: " + std::string(argv[0]) + " <ID>");
        log_error("Exemplo Docker: docker compose run --rm seek1 12345");

        return 1;
    
    }

    int id_busca;
    
    try {
    
        id_busca = std::stoi(argv[1]);
    
    }
    
    catch (const std::exception& e) {
        
        log_error("ID '" + std::string(argv[1]) + "' inválido. Deve ser um número inteiro.");
        
        return 1;
    
    }

    //#################################################################
    // 2. Definição dos caminhos e início dos logs
    //#################################################################
    
    const std::string dataDir = "/data/db";
    const std::string diretorio_hash_dados = dataDir + "/artigos.dat";
    const std::string btreeIdPath = dataDir + "/btree_id.idx";

    log_info("--- Iniciando Busca (seek1) ---");
    log_info("Buscando ID: " + std::to_string(id_busca));
    log_info("Usando Índice Primário (B+Tree): " + btreeIdPath);
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

    std::optional<Artigo> resultado;
    long blocos_lidos_indice = 0;
    long total_blocos_indice = 0;
    long duration_ms = 0;

    //#################################################################
    // 4. Execução da Busca
    //#################################################################

    try {
        
        BPlusTree<int> btree_id(btreeIdPath, TAMANHO_BLOCO_BTREE);

        GerenciadorArquivoDados gerenciador_dados_hash(diretorio_hash_dados, TAMANHO_BLOCO_LOGICO_DADOS);
        
        auto startTime = std::chrono::high_resolution_clock::now();

        std::vector<long> ids_bucket = btree_id.search(id_busca);

        // 2. Se o índice encontrou a localização o bucket ID
        if (!ids_bucket.empty()) {
            
            long id_bucket = ids_bucket[0];

            // 3. Busca o bucket no arquivo de DADOS
            BlocoDeDados* bucket = static_cast<BlocoDeDados*>(gerenciador_dados_hash.getPonteiroBloco(id_bucket));

            // 4. Procura registro no bucket
            for (size_t i = 0; i < bucket->contador_registros; ++i) {

                if (bucket->registros[i].id == id_busca) {

                    resultado = bucket->registros[i];

                    break;

                }

            }

        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        // Coleta as estatísticas
        
        blocos_lidos_indice = btree_id.getIndexBlocosLidos();
        total_blocos_indice = btree_id.getIndexTotalBlocos();
        
    }
    
    catch (const std::exception& e) {
        
        std::cerr << "Erro Fatal durante a busca: " << e.what() << std::endl;
        
        return 1;
    
    }

    //#################################################################
    // 5. Relatório de Resultados e Estatísticas
    //#################################################################

    if (resultado) {
    
        log_info("--- Registro Encontrado ---");
    
        printArtigo(resultado.value());
    
    }
    
    else {
    
        log_info("--- Registro com ID " + std::to_string(id_busca) + " não encontrado. ---");
    
    }

    log_info("\n--- Estatísticas da Operação (seek1) ---");
    log_info("Tempo total de execução: " + std::to_string(duration_ms) + " ms");
    log_info("Arquivo de Índice Primário: " + btreeIdPath);
    
    log_info("  - Blocos lidos (Índice): " + std::to_string(blocos_lidos_indice));
    log_info("  - Total de blocos (Índice): " + std::to_string(total_blocos_indice));
    
    return 0;

}