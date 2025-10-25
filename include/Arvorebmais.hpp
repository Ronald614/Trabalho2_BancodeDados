#ifndef ARVOREBMAIS_HPP
#define ARVOREBMAIS_HPP

#include <string>
#include <vector>
#include "GerenciaBlocos.hpp" // Assumindo que você renomeou

#pragma pack(push, 1) 
// Structs No e cabecalho (sem alterações)
struct No
{
    bool ehFolha;
    int numChaves;
    long proximo; // Ponteiro para o próximo nó folha (lista ligada)
    long selfId; // ID do bloco deste nó no arquivo
    std::vector<int> vetorChaves;       // vetor de chaves do nó atual
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
#pragma pack(pop) 

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

    // usado para escrever e ler nós do disco
    void escreverCabecalho();
    void lerCabecalho();
    void serializaNo(const No &no, char *buffer);   // serializar o no
    void deserializaNo(const char *buffer, No &no); // desserializar o no
    void escreverNo(No *no);                        // escrever no disco
    long getNovoId();                               // retorna novo id com base no total de blocos
    No *lerNo(long idBloco, No *no);                // ler do disco            

    // Funções de Inserção
    void splitChild(No *parent, int childIndex);           
    void insertNonFull(No *no, int key, long dataPointer);         

    void removeRecursivo(No* noAtual, int key);
    int findKeyIndex(No* no, int key);
    void removeFromLeaf(No* folha, int keyIndex);
    void removeFromInternal(No* interno, int keyIndex);

    void fillNode(No* pai, int indexFilho);
    void borrowFromPrev(No* pai, int indexFilho);
    void borrowFromNext(No* pai, int indexFilho);
    void merge(No* pai, int indexFilho);
    int getPred(No* no, int index);
    int getSucc(No* no, int index);

    void printintree(No *No, int level);        
    
public:
    // Construtor
    BPlusTreeInt(const std::string &nomeArquivo, size_t tamanhoBloco); 
    
    // Inserção e Busca
    void insert(int chave, long apontadorDados);                       
    long search(int chave);                                            
    void remove(int chave);

    // Impressão
    void printintree();                                                
};

#endif // ARVOREBMAIS_HPP
