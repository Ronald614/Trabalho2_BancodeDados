#ifndef BLOCKMANAGER_HPP
#define BLOCKMANAGER_HPPt

#include <fstream>
#include <string>

// Gerencia a leitura e escrita de blocos fixos em um arquivo no disco.
class BlockManager {
private:
    std::string filename;      // Caminho para o arquivo no disco.
    std::fstream fileStream;   // O stream do arquivo para operações de I/O.
    const size_t blockSize;    // Tamanho de cada bloco em bytes.

public:
    //Construtor que abre (ou cria) o arquivo de banco de dados.
    BlockManager(const std::string& db_filename, size_t block_size);

    // Destrutor que fecha o arquivo.
    ~BlockManager();

    // Lê o conteúdo de um bloco específico do disco para um buffer.
    void readBlock(long blockId, char* buffer);

   // Escreve o conteúdo de um buffer em um bloco específico no disco.
    void writeBlock(long blockId, const char* buffer);

    // Aloca um novo bloco no final do arquivo e retorna seu ID.
    long allocateBlock();

   // Retorna o número total de blocos atualmente alocados no arquivo.
    long getBlockCount();

    // Retorna o tamanho configurado para os blocos (em bytes).
    size_t getBlockSize() const;
};

#endif // BLOCKMANAGER_HPP

