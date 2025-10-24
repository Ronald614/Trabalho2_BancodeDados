#include "GerenciaBlocos.hpp"
#include <stdexcept>
#include <fstream>
#include <string>

//construtor
GerenciaBlocos::GerenciaBlocos(const std::string& nomeArquivo, size_t tamanhoBloco)
    : nomeArquivo(nomeArquivo), tamanhoBloco(tamanhoBloco) {
    
    this->fileStream.open(this->nomeArquivo, std::ios::in | std::ios::out | std::ios::binary);  //tenta abrir para leitura e escrita

    if(!this->fileStream.is_open()){//verifica se conseguiu abrir o arquivo
        this->fileStream.clear();//limpa as flags de erro

        //ja que nao conseguiu abrir tenta escrever (forca a criacao)
        if(!fileStream.is_open()){
            this->fileStream.open(this->nomeArquivo, std::ios::out | std::ios::binary); 
            if (!fileStream.is_open()) {
            // Se AINDA não abriu, é porque não conseguiu criar.
            // AGORA sim, você pode lançar o erro.
            throw std::runtime_error("Erro: Nao foi possivel criar o arquivo: " + this->nomeArquivo);
            }
        }
        this->fileStream.close();//fechamos o arquivo no modo somente para escrita

        
        //tenta abrir de novo para escrita e leitura
        this->fileStream.open(this->nomeArquivo, std::ios::in | std::ios::out | std::ios::binary); 

        if(!fileStream.is_open()){
        this->fileStream.open(this->nomeArquivo, std::ios::out | std::ios::binary); 
           if (!fileStream.is_open()) {
            // Se AINDA não abriu, é porque não conseguiu criar.
            // AGORA sim lançar o erro
                throw std::runtime_error("Erro: Nao foi possivel criar o arquivo: " + this->nomeArquivo);
            }
        }
    }
}

//destrutor
GerenciaBlocos::~GerenciaBlocos() {
    if (fileStream.is_open()) {//se esta aberto fecha o arquivo
        fileStream.close();
    }
}

// métodos para modificar e ler o arquivo
    
// Lê o conteúdo de um bloco específico do disco para um buffer.
void GerenciaBlocos::lerBloco(long idBloco, char* buffer) {
    // Calcula a posição exata do bloco no arquivo em bytes.
    long posicao = idBloco * this->tamanhoBloco; //achar a posicao e achar a posicao do bloco

    fileStream.clear(); // limpa flags antigas antes de mover ponteiro
    fileStream.seekg(posicao, std::ios::beg);// O ponteiro de leitura é movido para o início do bloco desejado.
    fileStream.read(buffer, this->tamanhoBloco);//finalmente le o bloco

    if (fileStream.fail() && !fileStream.eof()) {//se falhou mas nao e final de arquivo(falhar real)
        throw std::runtime_error("Erro ao ler o bloco " + std::to_string(tamanhoBloco) + " do arquivo.");
    }
}

// Escreve o conteúdo de um buffer em um bloco específico no disco.
void GerenciaBlocos::escreveBloco(long idBloco, const char* buffer) {
    // Calcula a posição exata para a escrita.
    long position = idBloco * this->tamanhoBloco;

    fileStream.clear(); // limpa flags antigas antes de mover o ponteiro
    fileStream.seekp(position, std::ios::beg);// O ponteiro de escrita é movido para o início do bloco desejado.
    fileStream.write(buffer, this->tamanhoBloco);

    if (fileStream.fail()) {
        throw std::runtime_error("Erro ao escrever no bloco " + std::to_string(idBloco) + " do arquivo.");
    }
    // Garante que os dados sejam escritos no disco, cumprindo os requisitos de persistência.
    fileStream.flush();

}


long GerenciaBlocos::getTamanhoArquivo() {
    //stream temporario para checar o tamanho do arquivo
    std::ifstream file(this->nomeArquivo, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        // Se não conseguiu abrir (provavelmente não existe), o tamanho é 0.
        return 0;
    }
    //posicao final do arquivo
    long size = file.tellg();
    file.close();
    return size;
}