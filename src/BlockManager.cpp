#include "BlockManager.hpp"
#include <stdexcept>

//construtor
BlockManager::BlockManager(const std::string& db_filename, size_t block_size)
    : filename(db_filename), blockSize(block_size) {
    
    // Tenta abrir o arquivo para leitura e escrita binária.
    fileStream.open(filename, std::ios::in | std::ios::out | std::ios::binary);
    
    // Se a abertura falhar (provavelmente porque o arquivo não existe),
    // cria o arquivo e tenta abri-lo novamente.
    if (!fileStream.is_open()) {
        fileStream.open(filename, std::ios::out | std::ios::binary);
        fileStream.close();
        fileStream.open(filename, std::ios::in | std::ios::out | std::ios::binary);
        
        if (!fileStream.is_open()) {
            throw std::runtime_error("Erro fatal: Nao foi possivel abrir ou criar o arquivo: " + filename);
        }
    }
}

//destrutor
BlockManager::~BlockManager() {
    if (fileStream.is_open()) {
        fileStream.close();
    }
}

// métodos

          
// Lê o conteúdo de um bloco específico do disco para um buffer.
void BlockManager::readBlock(long blockId, char* buffer) {
    // Calcula a posição exata do bloco no arquivo em bytes.
    long offset = blockId * blockSize;
    fileStream.seekg(offset, std::ios::beg);// O ponteiro de leitura é movido para o início do bloco desejado.
    fileStream.read(buffer, blockSize);

    if (fileStream.fail() && !fileStream.eof()) {
        throw std::runtime_error("Erro ao ler o bloco " + std::to_string(blockId) + " do arquivo.");
    }

}

// Escreve o conteúdo de um buffer em um bloco específico no disco.
void BlockManager::writeBlock(long blockId, const char* buffer) {
    // Calcula a posição exata para a escrita.
    long position = blockId * blockSize;
    fileStream.seekp(position, std::ios::beg);
    fileStream.write(buffer, blockSize);

    if (fileStream.fail()) {
        throw std::runtime_error("Erro ao escrever no bloco " + std::to_string(blockId) + " do arquivo.");
    }
    // Garante que os dados sejam escritos no disco, cumprindo os requisitos de persistência.
    fileStream.flush();
}


// Retorna o número total de blocos atualmente alocados no arquivo.
long BlockManager::getBlockCount() {
    fileStream.seekg(0, std::ios::end);
    long endPosition = fileStream.tellg();
    // O número de blocos é o tamanho total do arquivo dividido pelo tamanho de um bloco.
    return endPosition / blockSize;
}

long BlockManager::allocateBlock() {
    // A alocação mais simples é no final do arquivo. O ID do novo bloco
    // será o número atual de blocos.
    return getBlockCount();
}

size_t BlockManager::getBlockSize() const {
    return blockSize;
}