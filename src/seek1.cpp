// Módulos C++
#include <iostream>
#include <string>
#include <stdexcept>
#include <chrono>
#include <optional>
#include <iomanip>
#include <fstream>

// Nossos módulos
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

    //#################################################################
    // 1. Verificação de entrada
    //#################################################################
    if (argc != 2) {

        std::cerr << "Erro: Uso incorreto." << std::endl;
        std::cerr << "Uso: " << argv[0] << " <ID>" << std::endl;
        std::cerr << "Exemplo Docker: docker compose run --rm seek1 12345" << std::endl;
        
        return 1;
    
    }

    int id_busca;
    
    try {
    
        id_busca = std::stoi(argv[1]);
    
    }
    
    catch (const std::exception& e) {
        
        std::cerr << "Erro: ID '" << argv[1] << "' inválido. Deve ser um número inteiro." << std::endl;
        
        return 1;
    
    }

    //#################################################################
    // 2. Definição dos caminhos e início dos logs
    //#################################################################
    
    const std::string dataDir = "/data/db";
    const std::string diretorio_hash_dados = dataDir + "/artigos.dat";
    const std::string btreeIdPath = dataDir + "/btree_id.idx";

    std::cout << "--- Iniciando Busca (seek1) ---" << std::endl;
    std::cout << "Buscando ID: " << id_busca << std::endl;
    std::cout << "Usando Índice Primário (B+Tree): " << btreeIdPath << std::endl;
    std::cout << "Lendo de Arquivo de Dados (Hash): " << diretorio_hash_dados << std::endl;

    //#################################################################
    // 3. Configuração dos Gerenciadores
    //#################################################################

    int tamanho_bloco_os = obter_tamanho_bloco_fs("/data"); 
    
    if (tamanho_bloco_os <= 0) {
        
        tamanho_bloco_os = 4096;
    
    }

    const size_t TAMANHO_BRUTO_BUCKET = sizeof(BlocoDeDados);
    const size_t TAMANHO_BLOCO_LOGICO_DADOS = calcular_bloco_logico(TAMANHO_BRUTO_BUCKET, tamanho_bloco_os);
    
    size_t TAMANHO_BLOCO_BTREE = 4096;
    
    std::cout << "Usando Bloco Lógico de Dados: " << TAMANHO_BLOCO_LOGICO_DADOS << " bytes." << std::endl;
    std::cout << "Usando Bloco de Índice: " << TAMANHO_BLOCO_BTREE << " bytes." << std::endl;

    std::optional<Artigo> resultado;
    long blocos_lidos_indice = 0;
    long total_blocos_indice = 0;
    long blocos_lidos_dados = 0;
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
        blocos_lidos_dados = gerenciador_dados_hash.obterBlocosLidos();

    }
    
    catch (const std::exception& e) {
        
        std::cerr << "Erro Fatal durante a busca: " << e.what() << std::endl;
        
        return 1;
    
    }

    //#################################################################
    // 5. Relatório de Resultados e Estatísticas
    //#################################################################

    if (resultado) {

        std::cout << "\n--- Registro Encontrado ---" << std::endl;
        printArtigo(resultado.value());

    }
    
    else {

        std::cout << "\n--- Registro com ID " << id_busca << " não encontrado. ---" << std::endl;
    
    }

    std::cout << "\n--- Estatísticas da Operação (seek1) ---" << std::endl;
    std::cout << "Tempo total de execução: " << duration_ms << " ms" << std::endl;
    std::cout << "------------------------------------" << std::endl;
    std::cout << "Arquivo de Índice Primário (" << btreeIdPath << "):" << std::endl;
    std::cout << "  - Blocos lidos: " << blocos_lidos_indice << std::endl;
    std::cout << "  - Total de blocos: " << total_blocos_indice << std::endl;
    std::cout << "------------------------------------" << std::endl;
    std::cout << "Arquivo de Dados (" << diretorio_hash_dados << "):" << std::endl;
    std::cout << "  - Blocos lidos: " << blocos_lidos_dados << std::endl;

    return 0;

}