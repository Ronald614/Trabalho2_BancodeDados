// Módulos C++.
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <vector>

// Módulos feitos.
#include "Parser.hpp"
#include "OSInfo.hpp"
#include "gerenciador_de_blocos.hpp"
#include "hash_extensivel.hpp"
#include "bloco_de_dados.hpp" 
#include "Arvorebmais.hpp"
#include "GerenciaBlocos.hpp"

int main(int argc, char* argv[]) {

    //##################################################################################################################################
    // 1. Verificação de entrada.
    //##################################################################################################################################
    
    if (argc != 2) {
    
        std::cerr << "[upload] Uso incorreto." << std::endl;
        std::cerr << "Uso: " << argv[0] << " <caminho_para_o_arquivo_csv>" << std::endl;
        std::cerr << "Comando esperado: docker compose run --rm upload arquivo_entrada.csv" << std::endl;
    
        return 1;
    
    }

    //##################################################################################################################################
    // 2. Definição dos caminhos dos arquivos de dados.
    //##################################################################################################################################

    std::string nome_arquivo = argv[1];
    std::string diretorio_csv = "/data/" + nome_arquivo;

    const std::string dataDir = "/data/db"; 
    const std::string diretorio_hash = dataDir + "/artigos.dat";
    const std::string diretorio_hash_dir = dataDir + "/hash_dir.dat";
    const std::string btreeIdPath = dataDir + "/btree_id.idx";
    const std::string btreeTituloPath = dataDir + "/btree_titulo.idx";

    //##################################################################################################################################
    // 3. Iniciar medição de tempo e logs.
    //##################################################################################################################################
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    std::cout << "--- Iniciando Carga de Dados (Upload) ---" << std::endl;
    std::cout << "Arquivo CSV de entrada: " << diretorio_csv << std::endl;
    std::cout << "Arquivos de saída:" << std::endl;
    std::cout << "  - Dados (Hash): " << diretorio_hash << std::endl;
    std::cout << "  - Índice Primário (B+Tree ID): " << btreeIdPath << std::endl;
    std::cout << "  - Índice Secundário (B+Tree Título): " << btreeTituloPath << std::endl;

    //##################################################################################################################################
    // 4. Obter informações do sistema e calcular tamanhos de bloco.
    //##################################################################################################################################

    int tamanho_bloco_os = obter_tamanho_bloco_fs("/data"); 
    
    if (tamanho_bloco_os <= 0) { 
    
        std::cerr << "[upload] Nao foi possivel determinar o tamanho do bloco do S.O. Usando 4096." << std::endl;
    
        tamanho_bloco_os = 4096; 
    
    }
    
    const size_t TAMANHO_BRUTO_BUCKET = sizeof(BlocoDeDados);
    const size_t TAMANHO_BLOCO_LOGICO_DADOS = calcular_bloco_logico(TAMANHO_BRUTO_BUCKET, tamanho_bloco_os);
    const size_t TAMANHO_BLOCO_BTREE = 4096;
    
    // Logs
    std::cout << "Tamanho do bloco do S.O. em /data: " << tamanho_bloco_os << " bytes." << std::endl;
    std::cout << "Tamanho de cada registro (sizeof(Artigo)): " << sizeof(Artigo) << " bytes." << std::endl;
    std::cout << "Registros por Bloco Lógico (definido): " << CAPACIDADE_BUCKET << std::endl;
    std::cout << "Tamanho Bruto do Bucket (sizeof(BlocoDeDados)): " << TAMANHO_BRUTO_BUCKET << " bytes." << std::endl;
    std::cout << "Tamanho Lógico do Bloco de Dados Hash (arredondado): " << TAMANHO_BLOCO_LOGICO_DADOS << " bytes." << std::endl;
    std::cout << "Tamanho do Bloco para B+Tree ID: " << TAMANHO_BLOCO_BTREE << " bytes." << std::endl;


    //##################################################################################################################################
    // 5. Inicializar Gerenciadores de Arquivos e Estruturas de Dados.
    //##################################################################################################################################

    try {

        GerenciadorDeBlocos gerenciador_dados_hash(diretorio_hash, TAMANHO_BLOCO_LOGICO_DADOS);
        GerenciadorDeBlocos gerenciador_dir_hash(diretorio_hash_dir, tamanho_bloco_os);

        GerenciaBlocos gerenciador_btree_id(btreeIdPath, TAMANHO_BLOCO_BTREE);
        BPlusTreeInt btree_id(btreeIdPath, TAMANHO_BLOCO_BTREE);

        ArquivoHashExtensivel arquivo_hash(gerenciador_dados_hash, gerenciador_dir_hash, btree_id);

        arquivo_hash.inicializar();

    //##################################################################################################################################
    // 6. Abrir e processar o arquivo CSV.
    //##################################################################################################################################
    
        std::ifstream arquivo_entrada(diretorio_csv);
        
        if (!arquivo_entrada.is_open()) {
        
            std::cerr << "[upload] Erro Fatal: Nao foi possivel abrir o arquivo de entrada: " << diretorio_csv << std::endl;
        
            return 1;
        
        }

        std::string linha;
        long contador_linhas_processadas = 0;
        long contador_linhas_ignoradas = 0;
        std::cout << "Processando registros..." << std::endl;

        while (std::getline(arquivo_entrada, linha)) {

            if (linha.empty()) { 
                
                continue; 
            
            }
            
            Artigo artigo;
            
            if (parseCSVLinha(linha, artigo)) {
            
                try {

                    arquivo_hash.inserir(artigo); 

                }
                
                catch (const std::exception& e) {
                
                    std::cerr << "[upload] Erro ao inserir artigo ID " << artigo.id << ": " << e.what() << std::endl;
                
                }
                
                contador_linhas_processadas++;
                
                if (contador_linhas_processadas % 10000 == 0) {
                
                    std::cout << "  ... " << contador_linhas_processadas << " registros processados." << std::endl;
                
                }
            }
            
            else {
            
                contador_linhas_ignoradas++;
            
            }
        
        }
        
        arquivo_entrada.close();
        
        std::cout << "Leitura do CSV concluída." << std::endl;

    //##################################################################################################################################
    // 9. Finalizar medição e imprimir relatório.
    //##################################################################################################################################
    
        auto endTime = std::chrono::high_resolution_clock::now();
        long duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        std::cout << "\n--- Carga de Dados Concluída ---" << std::endl;
        std::cout << "Tempo total de execução: " << duration_ms << " ms" << std::endl;
        std::cout << "Total de registros processados (inseridos): " << contador_linhas_processadas << std::endl;
        std::cout << "Total de linhas ignoradas (mal formatadas): " << contador_linhas_ignoradas << std::endl;
        
    //##################################################################################################################################
    // 10. Imprimir estatísticas de blocos.
    //##################################################################################################################################
    
        std::cout << "\nEstatísticas de I/O (Hash - Dados):" << std::endl;
        std::cout << "  - Blocos lidos: " << gerenciador_dados_hash.obterBlocosLidos() << std::endl;
        std::cout << "  - Blocos escritos: " << gerenciador_dados_hash.obterBlocosEscritos() << std::endl;
        std::cout << "  - Total de blocos no arquivo: " << gerenciador_dados_hash.obterNumeroTotalBlocos() << std::endl;

        std::cout << "Estatísticas de I/O (Hash - Diretório):" << std::endl;
        std::cout << "  - Blocos lidos: " << gerenciador_dir_hash.obterBlocosLidos() << std::endl;
        std::cout << "  - Blocos escritos: " << gerenciador_dir_hash.obterBlocosEscritos() << std::endl;
        std::cout << "  - Total de blocos no arquivo: " << gerenciador_dir_hash.obterNumeroTotalBlocos() << std::endl;

    } catch (const std::exception& e) {
        
        std::cerr << "\nErro Fatal durante a inicialização ou processamento: " << e.what() << std::endl;
        
        return 1;
    
    }

    return 0;

}

