#ifndef GERENCIABLOCOS_HPP
#define GERENCIABLOCOS_HPP

#include <fstream>
#include <string>

// Serve para alocar e ler blocos para o arquivo de indice
class GerenciaBlocos {
private:
    std::string nomeArquivo;      // Caminho do arquivo.
    std::fstream fileStream;   // O stream do arquivo para operações de I/O.
    const size_t tamanhoBloco;    // Tamanho de cada bloco em bytes.

public:
    //Construtor que abre (ou cria) o arquivo de banco de dados.
    GerenciaBlocos(const std::string& nome_arquivo_dados, size_t tamanhoBloco);

    // Destrutor que fecha o arquivo.
    ~GerenciaBlocos();

    // Lê o conteúdo de um bloco específico do disco para um buffer.
    void lerBloco(long idBloco, char* buffer);

   // Escreve o conteúdo de um buffer em um bloco específico no disco.
    void escreveBloco(long idBloco, const char* buffer);

    long retornaNovoId(); //Define um id com base no final do arquivo

    long getTamanhoArquivo();//Somente a posicao final do arquivo
};

#endif // GERENCIABLOCO_HPP

