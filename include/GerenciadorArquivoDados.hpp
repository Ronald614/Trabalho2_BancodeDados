#ifndef GERENCIADOR_ARQUIVO_DADOS_HPP
#define GERENCIADOR_ARQUIVO_DADOS_HPP

#include <string>
#include <cstddef> // Para size_t
#include <stdexcept>

/**
 * @class GerenciadorArquivoDados
 * @brief Gerencia a persistência de dados no disco usando Mapeamento de Memória (mmap).
 *
 * Esta classe permite alocar blocos de tamanho fixo no arquivo e acessar/manipular
 * seu conteúdo diretamente na memória, simulando um array contíguo de blocos.
 */
class GerenciadorArquivoDados {
private:
    std::string caminho_arquivo;
    int arquivo_fd;
    const size_t tamanho_bloco;
    void* mapa_memoria;
    size_t tamanho_total_arquivo;
    long blocos_lidos;
    long blocos_escritos;

public:
    /**
     * @brief Construtor. Abre/cria o arquivo e inicializa o mapeamento de memória (mmap).
     * @param caminho O caminho completo para o arquivo de dados.
     * @param tamanho O tamanho fixo de cada bloco em bytes.
     * @throws std::invalid_argument Se o tamanho do bloco for zero.
     * @throws std::runtime_error Em caso de falha ao abrir, criar, ajustar ou mapear o arquivo.
     */
    GerenciadorArquivoDados(const std::string& caminho, size_t tamanho);

    /**
     * @brief Destrutor. Sincroniza e fecha o mapeamento de memória (munmap) e o file descriptor.
     */
    ~GerenciadorArquivoDados();

    // Métodos de Alocação e Acesso

    /**
     * @brief Aloca um novo bloco no final do arquivo e estende o mapeamento de memória (mremap).
     * O novo bloco é inicializado com zeros.
     * @return O ID (índice) do bloco recém-alocado.
     * @throws std::runtime_error Em caso de falha ao estender (ftruncate) ou remapear (mremap).
     */
    size_t alocarNovoBloco();

    /**
     * @brief Aloca um número especificado de blocos em um arquivo vazio para inicialização.
     * @param num_blocos O número de blocos a serem alocados e preenchidos com zeros.
     * @throws std::runtime_error Se o arquivo não estiver vazio ou em falha de I/O.
     */
    void alocarBlocosEmMassa(size_t num_blocos);
    
    /**
     * @brief Retorna o ponteiro de memória direto para o início de um bloco.
     * Conta como uma operação de leitura para fins estatísticos.
     * @param id_bloco O ID (índice) do bloco desejado.
     * @return Um ponteiro void* para a posição do bloco na memória mapeada.
     * @throws std::out_of_range Se o ID do bloco estiver fora dos limites do arquivo.
     */
    void* getPonteiroBloco(size_t id_bloco);

    // Métodos de Sincronização

    /**
     * @brief Força a escrita de um único bloco do cache de memória para o disco (usando msync).
     * Conta como uma operação de escrita para fins estatísticos.
     * @param id_bloco O ID (índice) do bloco a ser sincronizado.
     */
    void sincronizarBloco(size_t id_bloco);

    /**
     * @brief Força a escrita de todo o arquivo mapeado para o disco (usando msync).
     * Reseta os contadores de blocos lidos e escritos.
     */
    void sincronizarArquivoInteiro();

    // Métodos de Informação

    /**
     * @brief Obtém o número total de blocos alocados no arquivo.
     * @return O número de blocos.
     */
    size_t obterNumeroTotalBlocos() const {
        return tamanho_total_arquivo / tamanho_bloco;
    }
    
    /**
     * @brief Obtém o número total de operações de acesso a bloco (leituras via getPonteiroBloco).
     * @return O contador de blocos lidos.
     */
    long obterBlocosLidos() const {
        return blocos_lidos;
    }

    /**
     * @brief Obtém o número total de operações de sincronização de bloco (escritas via msync).
     * @return O contador de blocos escritos.
     */
    long obterBlocosEscritos() const {
        return blocos_escritos;
    }
};

#endif // GERENCIADOR_ARQUIVO_DADOS_HPP