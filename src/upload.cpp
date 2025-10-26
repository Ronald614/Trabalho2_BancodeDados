// Módulos C++
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <vector>
#include <filesystem>

// Nossos módulos
#include "Log.hpp"
#include "config.hpp"
#include "Parser.hpp"
#include "OSInfo.hpp"
#include "GerenciadorArquivoDados.hpp"
#include "BlocoDeDados.hpp"
#include "ArquivoHashEstatico.hpp"
#include "ArvoreBMais.hpp"

int main(int argc, char* argv[]) {

    log_init();

    //#################################################################
    // 1. Verificação de entrada.
    //#################################################################
    
    if (argc != 2) {
    
        log_error("Uso incorreto.");
        log_error("Uso: " + std::string(argv[0]) + " <caminho_para_o_arquivo_csv>");
        log_error("Comando esperado: docker compose run --rm upload arquivo_entrada.csv");
        return 1;
    
    }

    //#################################################################
    // 2. Definição dos caminhos dos arquivos de dados.
    //#################################################################

    std::string nome_arquivo = argv[1];
    std::string diretorio_csv = "/data/" + nome_arquivo;

    const std::string dataDir = "/data/db";
    const std::string metaDir = dataDir + "/db.meta";
    const std::string diretorio_hash = dataDir + "/artigos.dat";
    const std::string btreeIdPath = dataDir + "/btree_id.idx";
    const std::string btreeTituloPath = dataDir + "/btree_titulo.idx";

    //#################################################################
    // 3. Iniciar medição de tempo e logs.
    //#################################################################
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    log_info("--- Iniciando Carga de Dados (Upload) ---");
    log_info("Arquivo CSV de entrada: " + diretorio_csv);
    log_info("Arquivos de saída:");
    log_info("  - Dados (Hash): " + diretorio_hash);
    log_info("  - Índice Primário (B+Tree ID): " + btreeIdPath);
    log_info("  - Índice Secundário (B+Tree Título): " + btreeTituloPath);
    
    if (std::filesystem::exists(diretorio_hash) || std::filesystem::exists(btreeIdPath) || std::filesystem::exists(btreeTituloPath)) {
        
        log_info("\nIniciando limpeza de arquivos de banco de dados antigos...");
        
        try {
            
            std::filesystem::remove(diretorio_hash);
            std::filesystem::remove(btreeIdPath);
            std::filesystem::remove(btreeTituloPath);
            
            log_info("Arquivos anteriores removidos com sucesso.");

        } 
        
        catch (const std::filesystem::filesystem_error& e) {
            
            log_warn("Nao foi possivel remover arquivos antigos. O programa tentara sobrescreve-los. Erro: " + std::string(e.what()));
            
        }
    }

    //#################################################################
    // 4. Obter informações do sistema e calcular tamanhos de bloco.
    //#################################################################

    int tamanho_bloco_os = obter_tamanho_bloco_fs("/data"); 
    
    if (tamanho_bloco_os <= 0) { 
    
        log_warn("Nao foi possivel determinar o tamanho do bloco do S.O. Usando 4096.");
        tamanho_bloco_os = 4096;
    
    }
    
    const size_t TAMANHO_BRUTO_BUCKET = sizeof(BlocoDeDados);
    const size_t TAMANHO_BLOCO_LOGICO_DADOS = calcular_bloco_logico(TAMANHO_BRUTO_BUCKET, tamanho_bloco_os);
    const size_t TAMANHO_BLOCO_BTREE = static_cast<size_t>(tamanho_bloco_os);

    // Logs
    log_debug("Tamanho do bloco do S.O. em /data: " + std::to_string(tamanho_bloco_os) + " bytes.");
    log_debug("Tamanho de cada registro (sizeof(Artigo)): " + std::to_string(sizeof(Artigo)) + " bytes.");
    log_debug("Registros por Bloco Lógico (definido): " + std::to_string(CAPACIDADE_BUCKET));
    log_debug("Tamanho Bruto do Bucket (sizeof(BlocoDeDados)): " + std::to_string(TAMANHO_BRUTO_BUCKET) + " bytes.");
    log_debug("Tamanho Lógico do Bloco de Dados Hash (arredondado): " + std::to_string(TAMANHO_BLOCO_LOGICO_DADOS) + " bytes.");

    
    log_info("Salvando metadados de bloco em: " + metaDir);

    std::ofstream meta_dados(metaDir, std::ios::binary | std::ios::trunc);
    if (!meta_dados.is_open()) {

        log_error("Falha fatal ao criar arquivo de metadados: " + metaDir);
        
        return 1;
    
    }
    
    // Salva o tamanho do bloco de dados e de indice
    meta_dados.write(reinterpret_cast<const char*>(&TAMANHO_BLOCO_LOGICO_DADOS), sizeof(size_t));
    meta_dados.write(reinterpret_cast<const char*>(&TAMANHO_BLOCO_BTREE), sizeof(size_t));
    
    meta_dados.close();

    //#################################################################
    // 5. Inicializar Gerenciadores de Arquivos e Estruturas de Dados.
    //#################################################################

    try {

        GerenciadorArquivoDados gerenciador_dados_hash(diretorio_hash, TAMANHO_BLOCO_LOGICO_DADOS);
        ArquivoHashEstatico arquivo_hash(gerenciador_dados_hash, NUM_BUCKETS_PRIMARIOS);

        arquivo_hash.inicializar();

        log_info("Inicializando Índice Primário (B+Tree ID)...");
        BPlusTree<int> btree_id(btreeIdPath, static_cast<size_t>(TAMANHO_BLOCO_BTREE));

        log_info("Inicializando Índice Secundário (B+Tree Título)...");
        BPlusTree<ChaveTitulo> btree_titulo(btreeTituloPath, static_cast<size_t>(TAMANHO_BLOCO_BTREE));

    //#################################################################
    // 6. Abrir e processar o arquivo CSV.
    //#################################################################
    
        std::ifstream arquivo_entrada(diretorio_csv);
        
        if (!arquivo_entrada.is_open()) {
        
            log_error("Nao foi possivel abrir o arquivo de entrada: " + diretorio_csv);
            return 1;
        
            return 1;
        
        }

        std::string linha;
        long contador_linhas_processadas = 0;
        long contador_linhas_ignoradas = 0;
        
        log_info("Processando registros...");

        while (std::getline(arquivo_entrada, linha)) {

            if (linha.empty()) { 
                
                continue; 
            
            }
            
            Artigo artigo;
            
            if (parseCSVLinha(linha, artigo)) {
            
                try {

                    size_t id_bloco_inserido = arquivo_hash.inserir(artigo);
                    btree_id.insert(artigo.id, id_bloco_inserido);

                    ChaveTitulo chave_titulo;
                    strncpy(chave_titulo.titulo, artigo.titulo, 300);
                    
                    btree_titulo.insert(chave_titulo, id_bloco_inserido);

                }
                
                catch (const std::exception& e) {
                
                    log_warn("Erro ao inserir artigo ID " + std::to_string(artigo.id) + ": " + std::string(e.what()));
                
                }
                
                contador_linhas_processadas++;

                if (contador_linhas_processadas % checkpoint_intervalo == 0) {
                    
                    log_info("  ... " + std::to_string(contador_linhas_processadas) + " registros processados. Sincronizando...");
                    
                    gerenciador_dados_hash.flushCheckpoint();
                    
                    btree_id.flush();
                    btree_titulo.flush();
                    
                    log_info("  ... Sincronização concluída.");
                
                }
                
                if (contador_linhas_processadas % 10000 == 0) {
                
                    log_info("  ... " + std::to_string(contador_linhas_processadas) + " registros processados.");
                
                }
                
            }
            
            else {
            
                contador_linhas_ignoradas++;
            
            }
        
        }
        
        arquivo_entrada.close();
        
        log_info("Leitura do CSV concluída.");

    //#################################################################
    // 9. Finalizar medição e imprimir relatório.
    //#################################################################
    
        auto endTime = std::chrono::high_resolution_clock::now();
        long duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        log_info("\n--- Carga de Dados Concluída ---");
        log_info("Tempo total de execução: " + std::to_string(duration_ms) + " ms");
        log_info("Total de registros processados (inseridos): " + std::to_string(contador_linhas_processadas));
        log_info("Total de linhas ignoradas (mal formatadas): " + std::to_string(contador_linhas_ignoradas));
        
    //#################################################################
    // 10. Imprimir estatísticas de blocos.
    //#################################################################
    
        log_info("\nEstatísticas de I/O (Hash - Dados): " + diretorio_hash);
        log_info("  - Blocos lidos: " + std::to_string(gerenciador_dados_hash.obterBlocosLidos()));
        log_info("  - Blocos escritos: " + std::to_string(gerenciador_dados_hash.obterBlocosEscritos()));
        log_info("  - Total de blocos no arquivo: " + std::to_string(gerenciador_dados_hash.obterNumeroTotalBlocos()));

        log_info("\nEstatísticas de I/O (B+Tree - ID): " + btreeIdPath);
        log_info("  - Blocos lidos: " + std::to_string(btree_id.getIndexBlocosLidos()));
        log_info("  - Blocos escritos: " + std::to_string(btree_id.getIndexBlocosEscritos()));
        log_info("  - Total de blocos no arquivo: " + std::to_string(btree_id.getIndexTotalBlocos()));

        log_info("\nEstatísticas de I/O (B+Tree - Título): " + btreeTituloPath);
        log_info("  - Blocos lidos: " + std::to_string(btree_titulo.getIndexBlocosLidos()));
        log_info("  - Blocos escritos: " + std::to_string(btree_titulo.getIndexBlocosEscritos()));
        log_info("  - Total de blocos no arquivo: " + std::to_string(btree_titulo.getIndexTotalBlocos()));

    } 
    
    catch (const std::exception& e) {
        
        log_error("Erro Fatal durante a inicialização ou processamento: " + std::string(e.what()));
        
        return 1;
    
    }

    return 0;

}