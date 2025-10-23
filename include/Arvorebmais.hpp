#ifndef ARVOREBMAIS_HPP
#define ARVOREBMAIS_HPP

#include <string>
#include <vector>
#include "GerenciaBlocos.hpp"

#pragma pack(push, 1) // Evitar padding nas structs No e header
// As structs a seguir representam os nós e o cabeçalho do arquivo de índice.
struct No
{
    bool ehFolha;
    int numChaves;
    long proximo;
    long selfId;
    std::vector<int> vetorChaves;
    std::vector<long> vetorApontadores; // filhos ou ponteiros para registros
    No(bool ehFolha = false) : ehFolha(ehFolha), numChaves(0), proximo(-1), selfId(-1) {}
};

// Como ler o bloco inicial do arquivo de indice
// importante para a posicao da raiz persistir
// e o tamanho de bloco original utilizado na insercao
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

    // assinaturas privadas

    // usado para escrever e ler nós do disco
    void escreverCabecalho();
    void lerCabecalho();
    void serializaNo(const No &no, char *buffer);   // serializar o no
    void deserializaNo(const char *buffer, No &no); // desserializar o no
    void escreverNo(No *no);                        // escrever no disco
    No *lerNo(long idBloco, No *no);                // ler do disco

    // usado na logica da arvore
    void splitChild(No *parent, int childIndex);           // dividir filho (usado na insercao)
    void insertNonFull(No *No, int key, long dataPointer); // inserir em no nao cheio (usado na insercao)
    void printintree(No *No, int level);                   // imprimir arvore recursivamente

public:
    // construtor e métodos públicos
    BPlusTreeInt(const std::string &nomeArquivo, size_t tamanhoBloco); // construtor
    void insert(int chave, long apontadorDados);                       // inserir chave com ponteiro para dado
    long search(int chave);                                            // buscar chave
    void printintree();                                                // imprimir árvore
};
#endif // ARVOREBMAIS_HPP