#ifndef ARVOREBMAISTITULO_HPP
#define ARVOREBMAISTITULO_HPP

#include <string>
#include <vector>
#include <cstring> // Para memset e strncmp
#include "GerenciaBlocos.hpp" 

// --- Novas Structs e Cabeçalho Empacotados ---
#pragma pack(push, 1) // Evitar padding nas structs abaixo

// Struct para a Chave (Título)
struct ChaveTitulo {
    char titulo[300]; 

    // Operadores de comparação (essenciais para a B+ Tree)
    bool operator<(const ChaveTitulo& other) const {
        return strncmp(titulo, other.titulo, 300) < 0;
    }
    bool operator>(const ChaveTitulo& other) const {
         return strncmp(titulo, other.titulo, 300) > 0;
    }
     bool operator==(const ChaveTitulo& other) const {
         return strncmp(titulo, other.titulo, 300) == 0;
     }
     bool operator>=(const ChaveTitulo& other) const {
         return strncmp(titulo, other.titulo, 300) >= 0;
     }
     // Construtor padrão para inicializar com zeros
     ChaveTitulo() { memset(titulo, 0, 300); } 

     // Construtor para facilitar a criação a partir de std::string ou const char*
     ChaveTitulo(const char* str) {
         strncpy(titulo, str, 299); 
         titulo[299] = '\0'; 
     }
     ChaveTitulo(const std::string& str) : ChaveTitulo(str.c_str()) {}
};

// Estrutura do cabeçalho do arquivo de índice
// idRaiz: identifica o bloco onde está a raiz da árvore B+
// numBlocos: número total de blocos alocados no arquivo
// tamanhoBloco: tamanho de cada bloco em bytes
struct cabecalhoTitulo 
{
    long idRaiz;
    int numBlocos;
    long tamanhoBloco;
};

#pragma pack(pop) // Restaura o alinhamento padrão

// As structs a seguir representam os nos e o cabeçalho do arquivo de indice.
struct NoTitulo 
{
    bool ehFolha;
    int numChaves;
    long proximo; // Ponteiro para o próximo nó folha (lista ligada)
    long selfId; // ID do bloco deste nó no arquivo
    std::vector<ChaveTitulo> vetorChaves; // vetor de chaves do nó atual
    std::vector<long> vetorApontadores; // filhos ou ponteiros para registros
    
    NoTitulo(bool ehFolha = false) : ehFolha(ehFolha), numChaves(0), proximo(-1), selfId(-1) {}
};


class BPlusTreeTitulo 
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
    void serializaNo(const NoTitulo &no, char *buffer);  
    void deserializaNo(const char *buffer, NoTitulo &no); 
    void escreverNo(NoTitulo *no);                        // escrever nó no arquivo
    long getNovoId();                               // retorna novo id com base no total de blocos
    NoTitulo *lerNo(long idBloco, NoTitulo *no);                // ler no do arquivo               

    // usado na logica da arvore
    void splitChild(NoTitulo *parent, int childIndex);           // dividir filho (usado na insercao)
    void insertNonFull(NoTitulo *no, const ChaveTitulo& chave, long dataPointer); // inserir em no nao cheio (usado na insercao)

    // Funções de Remoção (Adaptadas - assinaturas simplificadas por enquanto)
    void removeRecursivo(NoTitulo* noAtual, const ChaveTitulo& chave, long dataPointer); 
    int findKeyIndex(NoTitulo* no, const ChaveTitulo& chave); 
    void removeFromLeaf(NoTitulo* folha, int keyIndex, long dataPointer); 
    void removeFromInternal(NoTitulo* interno, int keyIndex, long dataPointerOriginal);
    std::pair<ChaveTitulo, long> getPred(NoTitulo* no, int index);
    std::pair<ChaveTitulo, long> getSucc(NoTitulo* no, int index);
    void fillNode(NoTitulo* pai, int indexFilho); 
    void borrowFromPrev(NoTitulo* pai, int indexFilho); 
    void borrowFromNext(NoTitulo* pai, int indexFilho); 
    void merge(NoTitulo* pai, int indexFilho); 
    
public:
    // construtor e métodos públicos
    BPlusTreeTitulo(const std::string &nomeArquivo, size_t tamanhoBloco); // construtor
    void insert(const ChaveTitulo& chave, long dataPointer);                       // inserir chave com apontador para dado
    std::vector<long> search(const ChaveTitulo& chave);                                            // buscar chave
    // Remove um par específico (Título, ID) - mais simples para duplicatas
    void remove(const ChaveTitulo& chave, long dataPointer); 
};

#endif // ARVOREBMAISTITULO_HPP