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
#include "ArquivoHashEstatico.hpp"

/**
 * @brief Programa findrec: Busca um registro pelo ID no arquivo de hash.
 *
 * Especificação: "findrec <ID>: Programa que busca diretamente no arquivo
 * de dados por um registro com o ID informado e, se existir, retorna os
 * campos do registro, a quantidade de blocos lidos para encontrá-lo e
 * a quantidade total de blocos do arquivo de dados" 
 */

int main(int argc, char* argv[]) {

    log_init();

    //#################################################################
    // 1. Verificação de entrada
    //#################################################################

    if (argc != 2) {

        log_error("Uso incorreto.");
        log_error("Uso: " + std::string(argv[0]) + " <ID>");
        log_error("Exemplo Docker: docker compose run --rm findrec 12345");
        return 1;

    }

    int id_busca;

    try {

        id_busca = std::stoi(argv[1]);

    } catch (const std::exception& e) {

        log_error("ID '" + std::string(argv[1]) + "' inválido. Deve ser um número inteiro.");

        return 1;

    }

    if (id_busca <= 0) {
        
        log_error("ID '" + std::string(argv[1]) + "' inválido. O ID deve ser um número positivo maior que zero.");
        
        return 1;
    
    }

    //#################################################################
    // 2. Definição dos caminhos e início dos logs
    //#################################################################
    
    const std::string dataDir = "/data/db";
    const std::string diretorio_hash_dados = dataDir + "/artigos.dat";

    log_info("--- Iniciando Busca (findrec) ---");
    log_info("Buscando ID: " + std::to_string(id_busca));
    log_info("Arquivo de Dados (Hash): " + diretorio_hash_dados);
    
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
    long blocos_lidos = 0;
    size_t total_blocos_dados = 0;
    long duration_ms = 0;

    //#################################################################
    // 4. Execução da Busca
    //#################################################################

    try {

        GerenciadorArquivoDados gerenciador_dados_hash(diretorio_hash_dados, TAMANHO_BLOCO_LOGICO_DADOS);

        ArquivoHashEstatico arquivo_hash(gerenciador_dados_hash, NUM_BUCKETS_PRIMARIOS);

        auto startTime = std::chrono::high_resolution_clock::now();

        resultado = arquivo_hash.buscar(id_busca);

        auto endTime = std::chrono::high_resolution_clock::now();
        duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        blocos_lidos = gerenciador_dados_hash.obterBlocosLidos();
        total_blocos_dados = gerenciador_dados_hash.obterNumeroTotalBlocos();

    }
    
    catch (const std::exception& e) {
        
        log_error("Erro Fatal durante a busca: " + std::string(e.what()));
        
        return 1;
    
    }

    //#################################################################
    // 5. Relatório de Resultados
    //#################################################################

    if (resultado) {
    
        log_info("--- Registro Encontrado ---");
        printArtigo(resultado.value());
    
    }
    
    else {
    
        log_info("--- Registro com ID " + std::to_string(id_busca) + " não encontrado. ---");
    
    }

    //#################################################################
    // 6. Relatório de Estatísticas
    //#################################################################
    
    log_info("\n--- Estatísticas da Operação (findrec) ---");
    log_info("Tempo total de execução: " + std::to_string(duration_ms) + " ms");
    log_info("Arquivo de Dados: " + diretorio_hash_dados);
    log_info("  - Blocos lidos (Dados): " + std::to_string(blocos_lidos));
    log_info("  - Total de blocos (Dados): " + std::to_string(total_blocos_dados));

    return 0;
    
}