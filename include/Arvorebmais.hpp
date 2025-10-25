#ifndef ARVOREBMAIS_HPP
#define ARVOREBMAIS_HPP

#include <string>
#include <vector>
#include "GerenciaBlocos.hpp"

// As structs a seguir representam os nos e o cabeçalho do arquivo de indice.
struct No
{
    bool ehFolha;
    int numChaves;
    long proximo;
    long selfId;
    std::vector<int> vetorChaves;       // vetor de chaves do nó atual
    std::vector<long> vetorApontadores; // filhos ou ponteiros para registros
    No(bool ehFolha = false) : ehFolha(ehFolha), numChaves(0), proximo(-1), selfId(-1) {}
};

// Estrutura do cabeçalho do arquivo de índice
// idRaiz: identifica o bloco onde está a raiz da árvore B+
// numBlocos: número total de blocos alocados no arquivo
// tamanhoBloco: tamanho de cada bloco em bytes
#pragma pack(push, 1) // Evitar padding na struct cabecalho
struct cabecalho
{
    long idRaiz;
    int numBlocos;
    long tamanhoBloco;
};
#pragma pack(pop) // Restaura o alinhamento padrão

class BPlusTreeInt
{
private:
    // atributos privados
    int tamanhoBloco;           // tamanho do bloco em bytes, declarar antes de m !
    std::string nomeArquivo;    // nome do arquivo de banco de dados
    int m;                      // ordem da árvore B+ valor maximo de chaves por nó
    GerenciaBlocos gerenciador; // declarado aqui
    long idRaiz;                // Apontador pra raiz da arvore
    long totalBlocos;           // Total de blocos ja ocupado no arquivo
    

    // assinaturas privadas
    // usado para escrever e ler nós do disco
    void escreverCabecalho();       
    void lerCabecalho();
    void serializaNo(const No &no, char *buffer);  
    void deserializaNo(const char *buffer, No &no); 
    void escreverNo(No *no);                        // escrever nó no arquivo
    long getNovoId();                               // retorna novo id com base no total de blocos
    No *lerNo(long idBloco, No *no);                // ler no do arquivo

    // usado na logica da arvore
    void splitChild(No *parent, int childIndex);           // dividir filho (usado na insercao)
    void insertNonFull(No *No, int key, long dataPointer); // inserir em no nao cheio (usado na insercao)
    void printintree(No *No, int level);                   // imprimir arvore recursivamente (nao implementado)

public:
    // construtor e métodos públicos
    BPlusTreeInt(const std::string &nomeArquivo, size_t tamanhoBloco); // construtor
    void insert(int chave, long apontadorDados);                       // inserir chave com apontador para dado
    long search(int chave);                                            // buscar chave
    void printintree();                                                // imprimir árvore (nao implementado)
};

#endif // ARVOREBMAIS_HPP