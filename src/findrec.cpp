// Módulos C++
#include <iostream>
#include <string>
#include <stdexcept>
#include <chrono>
#include <optional>
#include <iomanip>

// Nossos módulos
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

    //#################################################################
    // 1. Verificação de entrada
    //#################################################################

    if (argc != 2) {

        std::cerr << "Erro: Uso incorreto." << std::endl;
        std::cerr << "Uso: " << argv[0] << " <ID>" << std::endl;
        std::cerr << "Exemplo Docker: docker compose run --rm findrec 12345" << std::endl;

        return 1;

    }

    int id_busca;

    try {

        id_busca = std::stoi(argv[1]);

    } catch (const std::exception& e) {

        std::cerr << "Erro: ID '" << argv[1] << "' inválido. Deve ser um número inteiro." << std::endl;

        return 1;

    }

    //#################################################################
    // 2. Definição dos caminhos e início dos logs
    //#################################################################
    
    const std::string dataDir = "/data/db";
    const std::string diretorio_hash_dados = dataDir + "/artigos.dat";

    std::cout << "--- Iniciando Busca (findrec) ---" << std::endl;
    std::cout << "Buscando ID: " << id_busca << std::endl;
    std::cout << "Arquivo de Dados (Hash): " << diretorio_hash_dados << std::endl; // 

    //#################################################################
    // 3. Configuração dos Gerenciadores
    //#################################################################

    // Obter o tamanho de bloco do S.O.
    int tamanho_bloco_os = obter_tamanho_bloco_fs("/data"); //
    if (tamanho_bloco_os <= 0) {
        std::cerr << "[findrec] Não foi possível determinar o tamanho do bloco. Usando 4096." << std::endl;
        tamanho_bloco_os = 4096;
    }

    // Calcular o tamanho lógico do bloco de dados (deve ser o mesmo do upload)
    const size_t TAMANHO_BRUTO_BUCKET = sizeof(BlocoDeDados);
    const size_t TAMANHO_BLOCO_LOGICO_DADOS = calcular_bloco_logico(TAMANHO_BRUTO_BUCKET, tamanho_bloco_os); //
    
    std::cout << "Usando Bloco Lógico de Dados: " << TAMANHO_BLOCO_LOGICO_DADOS << " bytes." << std::endl;

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
        
        std::cerr << "Erro Fatal durante a busca: " << e.what() << std::endl; // 
        
        return 1;
    
    }

    //#################################################################
    // 5. Relatório de Resultados
    //#################################################################

    if (resultado) {
    
        std::cout << "\n--- Registro Encontrado ---" << std::endl;
        printArtigo(resultado.value());
    
    }
    
    else {
    
        std::cout << "\n--- Registro com ID " << id_busca << " não encontrado. ---" << std::endl;
    
    }

    //#################################################################
    // 6. Relatório de Estatísticas
    //#################################################################
    std::cout << "\n--- Estatísticas da Operação ---" << std::endl;
    std::cout << "Tempo total de execução: " << duration_ms << " ms" << std::endl;
    std::cout << "Blocos lidos (Arquivo de Dados): " << blocos_lidos << std::endl;
    std::cout << "Total de blocos no Arquivo de Dados: " << total_blocos_dados << std::endl;

    return 0;
    
}