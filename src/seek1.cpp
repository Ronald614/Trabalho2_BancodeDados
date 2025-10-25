// Módulos C++.
#include <iostream>
#include <string>
#include <stdexcept>
#include <chrono>
#include <iomanip>
#include <optional>

// Módulos feitos.
#include "OSInfo.hpp"
#include "bloco_de_dados.hpp"
#include "gerenciador_de_blocos.hpp"
#include "Arvorebmais.hpp"
#include "Parser.hpp"

int main(int argc, char* argv[]) {

    //##################################################################################################################################
    // 1. Verificação de entrada.
    //##################################################################################################################################
    
    if (argc != 2) {

        std::cerr << "Erro: Uso incorreto." << std::endl;
        std::cerr << "Uso: " << argv[0] << " <ID>" << std::endl;
        std::cerr << "Comando esperado: docker compose run --rm seek1 12345" << std::endl;
        
        return 1;
    
    }

    int id_buscar;
    
    try {
    
        id_buscar = std::stoi(argv[1]);
    
    } catch (const std::exception& e) {
    
        std::cerr << "[seek1] ID inválido. Deve ser um número inteiro." << std::endl;
    
        return 1;
    
    }

    //##################################################################################################################################
    // 2. Definição dos caminhos dos arquivos de dados.
    //##################################################################################################################################

    const std::string dataDir = "/data/db"; 
    const std::string diretorio_hash = dataDir + "/artigos.dat";
    const std::string btreeIdPath = dataDir + "/btree_id.idx";

    //##################################################################################################################################
    // 3. Iniciar medição de tempo e logs.
    //##################################################################################################################################
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    std::cout << "--- Iniciando Busca (seek1) ---" << std::endl;
    std::cout << "Buscando ID: " << id_buscar << std::endl;
    std::cout << "Usando Índice Primário (B+Tree): " << btreeIdPath << std::endl;
    std::cout << "Lendo de Arquivo de Dados (Hash): " << diretorio_hash << std::endl;

    //##################################################################################################################################
    // 4. Obter informações do sistema e calcular tamanhos de bloco. (Consistente com upload)
    //##################################################################################################################################

    int tamanho_bloco_os = obter_tamanho_bloco_fs("/data"); 
    
    if (tamanho_bloco_os <= 0) { 
        
        tamanho_bloco_os = 4096; 
    
    }
    
    const size_t TAMANHO_BRUTO_BUCKET = sizeof(BlocoDeDados);
    const size_t TAMANHO_BLOCO_LOGICO_DADOS = calcular_bloco_logico(TAMANHO_BRUTO_BUCKET, tamanho_bloco_os);
    const size_t TAMANHO_BLOCO_BTREE = 4096;
    
    BPlusTreeInt btree_id(btreeIdPath, TAMANHO_BLOCO_BTREE);

    std::cout << "Tamanho Bloco Lógico de Dados (Hash): " << TAMANHO_BLOCO_LOGICO_DADOS << " bytes." << std::endl;
    std::cout << "Tamanho Bloco B+Tree: " << TAMANHO_BLOCO_BTREE << " bytes." << std::endl;

    //##################################################################################################################################
    // 5. Inicializar Estruturas e Realizar a Busca.
    //##################################################################################################################################

    try {
        
        BPlusTreeInt btree_id(btreeIdPath, TAMANHO_BLOCO_BTREE);

        GerenciadorDeBlocos gerenciador_dados_hash(diretorio_hash, TAMANHO_BLOCO_LOGICO_DADOS);

        long id_bucket = btree_id.search(id_buscar);

        std::optional<Artigo> artigo_encontrado = std::nullopt;

        if (id_bucket != -1) {
        
            BlocoDeDados* bucket = static_cast<BlocoDeDados*>(gerenciador_dados_hash.getPonteiroBloco(id_bucket));

            for (size_t i = 0; i < bucket->contador_registros; ++i) {
        
                if (bucket->registros[i].id == id_buscar) {
        
                    artigo_encontrado = bucket->registros[i];
        
                    break;
        
                }
        
            }
            
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        long duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        if (artigo_encontrado) {
        
            printArtigo(artigo_encontrado.value());
        
        }
        
        else {

            std::cout << "\nRegistro com ID " << id_buscar << " não encontrado." << std::endl;
        
        }

        //##################################################################################################################################
        // 6. Imprimir Estatísticas
        //##################################################################################################################################
    
        std::cout << "\n--- Estatísticas (seek1) ---" << std::endl;
        std::cout << "Tempo total de execução: " << duration_ms << " ms" << std::endl;
        
        std::cout << "\nEstatísticas do Arquivo de Índice Primário (" << btreeIdPath << "):" << std::endl << "algum dia vai ter" << std::endl;
    }
    
    catch (const std::exception& e) {
        
        std::cerr << "[seek1] Erro Fatal durante a busca: " << e.what() << std::endl;
        
        return 1;
    
    }

    return 0;
}