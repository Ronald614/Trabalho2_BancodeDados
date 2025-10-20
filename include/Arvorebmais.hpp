#ifndef ARVOREBMAIS_HPP
#define ARVOREBMAIS_HPP

#include <string>
#include <vector>
#include "BlockManager.hpp"


#pragma pack(push, 1) // Evitar padding nas structs node e header
class BPlusTreeInt {
private:
    //As structs a seguir representam os nós e o cabeçalho do arquivo de índice.
    struct Node {
        bool isLeaf;
        int numKeys;
        long next;
        long selfBlock;
        std::vector<int> keys;
        std::vector<long> childrenOrPointers; // filhos ou ponteiros para registros
        Node(bool leaf = false) : isLeaf(leaf), numKeys(0), next(-1), selfBlock(-1) {}
    };

    //Como ler o bloco inicial do arquivo de indice
    //importante para a posicao da raiz persistir 
    //e o tamanho de bloco original utilizado na insercao
    struct header{
        long root;
        int numBlocos;
        long blockSize;
    };


    // atributos privados
    int blockSize; // tamanho do bloco em bytes, declarar antes de m !
    std::string fileName; // nome do arquivo de banco de dados
    int m; // ordem da árvore B+
    BlockManager blockManager; // declarado aqui
    long rootBlock; // bloco raiz da árvore

    // assinaturas privadas
    //usado para escrever e ler nós do disco
    void serializeNode(const Node& node, char* buffer); //serializar o no
    void deserializeNode(const char* buffer, Node& node);//desserializar o no
    void writeNodeToDisk(Node* node);//escrever no disco
    Node* readNodeFromDisk(long blockNum, Node* node);//ler do disco
    

    // usado na logica da arvore
    void splitChild(Node* parent, int index, Node* child);// dividir filho (usado na insercao)
    void insertNonFull(Node* node, int key); // inserir em no nao cheio (usado na insercao)
    void printintree(Node* node, int level); // imprimir arvore recursivamente
   
public:
    // construtor e métodos públicos
    BPlusTreeInt(const std::string& filename, size_t blockSize);// construtor
    void insert(int key, long dataPointer); // inserir chave com ponteiro para dado
    bool search(int key);// buscar chave
    std::vector<int> rangeQuery(int lower, int upper);// consulta por intervalo
    void printintree(); // imprimir árvore
};
#pragma pack(pop) // Restaura o alinhamento padrão

#endif // ARVOREBMAIS_HPP