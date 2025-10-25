#include <iostream>
#include <string>
#include <stdexcept>
#include <chrono>
#include <optional>

// Nossos módulos
#include "Parser.hpp"
#include "OSInfo.hpp"
#include "gerenciador_de_blocos.hpp"
#include "hash_extensivel.hpp"
#include "Artigo.hpp"
#include "bloco_de_dados.hpp"
#include "Arvorebmais.hpp"
#include "GerenciaBlocos.hpp"

/**
 * @brief Programa findrec: Busca um registro pelo ID no arquivo de hash.
 *
 * Especificação: "findrec <ID>: Programa que busca diretamente no arquivo
 * de dados por um registro com o ID informado e, se existir, retorna os
 * campos do registro, a quantidade de blocos lidos para encontrá-lo e
 * a quantidade total de blocos do arquivo de dados" 
 */

int main(int argc, char* argv[]) {

    if (argc != 2) {
    
        std::cerr << "Erro: Uso incorreto." << std::endl;
        std::cerr << "Uso: " << argv[0] << " <ID>" << std::endl;
        std::cerr << "Exemplo Docker: ./bin/findrec 12345" << std::endl;
    
        return 1;
    
    }

    int id_busca;
    
    try {
    
        id_busca = std::stoi(argv[1]);
    
    } 
    
    catch (const std::exception& e) {
    
        std::cerr << "Erro: ID invalido: " << argv[1] << std::endl;
    
        return 1;
    
    }

    const std::string dataDir = "/data/db";
    const std::string diretorio_hash_dados = dataDir + "/artigos.dat";
    const std::string diretorio_hash_dir = dataDir + "/hash_dir.dat";
    const std::string btreeIdPath = dataDir + "/btree_id.idx";

    int tamanho_bloco_os = obter_tamanho_bloco_fs("/data");
    
    if (tamanho_bloco_os <= 0) {
    
        std::cerr << "[seek1] Nao foi possivel determinar o tamanho do bloco. Usando 4096." << std::endl;
    
        tamanho_bloco_os = 4096;
    
    }

    BPlusTreeInt btree_id(btreeIdPath, tamanho_bloco_os);

    const size_t TAMANHO_BRUTO_BUCKET = sizeof(BlocoDeDados);
    const size_t TAMANHO_BLOCO_LOGICO_DADOS = calcular_bloco_logico(TAMANHO_BRUTO_BUCKET, tamanho_bloco_os);

    std::optional<Artigo> resultado;
    long blocos_lidos_dados = 0;
    long blocos_lidos_dir = 0;
    size_t total_blocos_dados = 0;
    long duration_ms = 0;

    std::cout << "Buscando ID: " << id_busca << "..." << std::endl;
    std::cout << "Usando Bloco Lógico de Dados: " << TAMANHO_BLOCO_LOGICO_DADOS << " bytes." << std::endl;

    try {
        
        GerenciadorDeBlocos gerenciador_dados_hash(diretorio_hash_dados, TAMANHO_BLOCO_LOGICO_DADOS);
        
        GerenciadorDeBlocos gerenciador_dir_hash(diretorio_hash_dir, tamanho_bloco_os);

        ArquivoHashExtensivel arquivo_hash(gerenciador_dados_hash, gerenciador_dir_hash, btree_id);

        auto startTime = std::chrono::high_resolution_clock::now();
        
        resultado = arquivo_hash.buscar(id_busca);

        auto endTime = std::chrono::high_resolution_clock::now();
        duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        blocos_lidos_dados = gerenciador_dados_hash.obterBlocosLidos();
        blocos_lidos_dir = gerenciador_dir_hash.obterBlocosLidos();
        total_blocos_dados = gerenciador_dados_hash.obterNumeroTotalBlocos();

    }
    
    catch (const std::exception& e) {
        
        std::cerr << "[seek1] Erro fatal durante a busca: " << e.what() << std::endl;
        
        return 1;
    
    }

    if (resultado) {
    
        std::cout << "\n--- Registro Encontrado ---" << std::endl;
    
        printArtigo(resultado.value());
    }
    
    else {
        
        std::cout << "\n--- Registro com ID " << id_busca << " nao encontrado. ---" << std::endl;
    
    }

    std::cout << "\n--- Estatisticas da Operacao ---" << std::endl;
    std::cout << "Tempo de execucao: " << duration_ms << " ms" << std::endl;
    std::cout << "Blocos lidos (Arquivo de Dados): " << blocos_lidos_dados << std::endl;
    std::cout << "Blocos lidos (Arquivo de Diretorio): " << blocos_lidos_dir << std::endl;
    std::cout << "Total de blocos no Arquivo de Dados: " << total_blocos_dados << std::endl;

    return 0;

}

