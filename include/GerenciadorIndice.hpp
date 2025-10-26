#ifndef GERENCIADOR_INDICE_HPP
#define GERENCIADOR_INDICE_HPP

#include <string>
#include <fstream>
#include <cstddef>
#include <stdexcept>

/**
 * @class GerenciadorIndice
 * @brief Gerencia a leitura e escrita de blocos de dados de tamanho fixo 
 * em um arquivo binário, simulando acesso a disco.
 * * Mantém contadores de operações de I/O para análise de desempenho.
 */
class GerenciadorIndice {
private:
    std::string nomeArquivo;
    std::fstream fileStream;
    const size_t tamanhoBloco;
    long blocos_lidos;
    long blocos_escritos;

public:
    /**
     * @brief Construtor. Tenta abrir o arquivo. Se não existir, tenta criá-lo.
     * @param nomeArquivo O caminho para o arquivo binário.
     * @param tamanhoBloco O tamanho fixo de cada bloco em bytes.
     * @throws std::runtime_error Se não for possível abrir ou criar o arquivo.
     */
    GerenciadorIndice(const std::string& nomeArquivo, size_t tamanhoBloco);

    /**
     * @brief Destrutor. Garante que o arquivo seja fechado.
     */
    ~GerenciadorIndice();

    // Métodos de I/O

    /**
     * @brief Lê o conteúdo de um bloco específico do disco para um buffer fornecido.
     * @param idBloco O índice (ID) do bloco a ser lido (base 0).
     * @param buffer O ponteiro para o buffer onde o conteúdo será armazenado.
     * @throws std::runtime_error Em caso de erro de leitura no arquivo.
     */
    void lerBloco(long idBloco, char* buffer);

    /**
     * @brief Escreve o conteúdo de um buffer em um bloco específico no disco.
     * @param idBloco O índice (ID) do bloco a ser escrito (base 0).
     * @param buffer O ponteiro para o buffer que contém os dados a serem escritos.
     * @throws std::runtime_error Em caso de erro de escrita no arquivo.
     */
    void escreveBloco(long idBloco, const char* buffer);

    /**
     * @brief Força a escrita de todos os buffers pendentes para o disco.
     */
    void flush();

    // Métodos de Informação

    /**
     * @brief Obtém o tamanho total do arquivo em bytes.
     * @return O tamanho total do arquivo, ou 0 se o arquivo não puder ser aberto.
     */
    long getTamanhoArquivo();
    
    /**
     * @brief Obtém o número total de operações de leitura de bloco realizadas.
     * @return O contador de blocos lidos.
     */
    long getBlocosLidos() const;

    /**
     * @brief Obtém o número total de operações de escrita de bloco realizadas.
     * @return O contador de blocos escritos.
     */
    long getBlocosEscritos() const;
};

#endif // GERENCIADOR_INDICE_HPP