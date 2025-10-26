// Módulos C++
#include <iostream>
#include <string>
#include <stdexcept>
#include <chrono>
#include <optional>
#include <vector>
#include <cstring>
#include <set>

// Nossos módulos
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

    //#################################################################
    // 1. Verificação de entrada
    //#################################################################

    if (argc != 2) {
    
        std::cerr << "Erro: Uso incorreto." << std::endl;
        std::cerr << "Uso: " << argv[0] << " \"<Titulo a ser buscado>\"" << std::endl;
        std::cerr << "Exemplo Docker: docker compose run --rm seek2 \"Um Titulo Exato\"" << std::endl;
    
        return 1;
    
    }

    const std::string titulo_busca = argv[1];
    if (titulo_busca.length() > 299) {

        std::cerr << "Erro: Título muito longo. Máximo de 299 caracteres." << std::endl;
        return 1;
    
    }

    //#################################################################
    // 2. Definição dos caminhos e início dos logs
    //#################################################################
    const std::string dataDir = "/data/db";
    const std::string diretorio_hash_dados = dataDir + "/artigos.dat";
    const std::string btreeTituloPath = dataDir + "/btree_titulo.idx";

    std::cout << "--- Iniciando Busca (seek2) ---" << std::endl;
    std::cout << "Buscando Título: \"" << titulo_busca << "\"" << std::endl;
    std::cout << "Usando Índice Secundário (B+Tree): " << btreeTituloPath << std::endl;
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

    std::vector<Artigo> resultados;
    long blocos_lidos_indice = 0;
    long total_blocos_indice = 0;
    long blocos_lidos_dados = 0;
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
        blocos_lidos_dados = gerenciador_dados_hash.obterBlocosLidos();

    } catch (const std::exception& e) {

        std::cerr << "Erro Fatal durante a busca: " << e.what() << std::endl;
        
        return 1;
    
    }

    //#################################################################
    // 5. Relatório de Resultados e Estatísticas
    //#################################################################

    if (!resultados.empty()) {
    
        std::cout << "\n--- " << resultados.size() << " Registro(s) Encontrado(s) ---" << std::endl;
    
        for (const auto& artigo : resultados) {
    
            printArtigo(artigo);
    
        }

    }
    
    else {
        
        std::cout << "\n--- Nenhum registro com o Título \"" << titulo_busca << "\" foi encontrado. ---" << std::endl;
    
    }

    std::cout << "\n--- Estatísticas da Operação (seek2) ---" << std::endl;
    std::cout << "Tempo total de execução: " << duration_ms << " ms" << std::endl;
    std::cout << "------------------------------------" << std::endl;
    std::cout << "Arquivo de Índice Secundário (" << btreeTituloPath << "):" << std::endl;
    std::cout << "  - Blocos lidos: " << blocos_lidos_indice << std::endl;
    std::cout << "  - Total de blocos: " << total_blocos_indice << std::endl;
    std::cout << "------------------------------------" << std::endl;
    std::cout << "Arquivo de Dados (" << diretorio_hash_dados << "):" << std::endl;
    std::cout << "  - Blocos lidos: " << blocos_lidos_dados << std::endl;

    return 0;

}