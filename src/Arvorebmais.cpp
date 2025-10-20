// Bibliotecas padrão necessárias
#include <algorithm> // Para funções como std::lower_bound (útil na busca)
#include <iostream>  // Para std::cout (impressão na tela)
#include <vector>    // Para std::vector (nossas listas dinâmicas)
#include <cstring>   // Para memcpy
#include <math.h>    // Para floor (na definição de 'm')
#include "BlockManager.hpp"// Gerenciamento de blocos em disco
#include "Arvorebmais.hpp"  // Declaração da classe BPlusTreeInt

/*
 * ==========================================================
 * Implementação dos Métodos da Classe BPlusTreeInt
 * ==========================================================
 */

// Constructor
BPlusTreeInt::BPlusTreeInt(const std::string& filename, const size_t blockSize_arg)
    : blockSize(static_cast<int>(blockSize_arg)),
      fileName(filename),
      blockManager(filename, blockSize_arg),
      rootBlock(-1)
{
    char* buffer = new char[this->blockSize];
    try { // tentar ler o header do arquivo de índice
        // a arvore existe
        if (blockManager.getBlockCount() > 0) {

            blockManager.readBlock(0, buffer);
            header hdr;
            memcpy(&hdr, buffer, sizeof(header));
    
            // Verifica se o tamanho do bloco no arquivo bate com o fornecido
            if (hdr.blockSize != this->blockSize) {
                delete[] buffer; 
                throw std::runtime_error("Erro: O tamanho do bloco fornecido é inconsistente com o do arquivo!");
            }
            this->rootBlock = hdr.root;

        } else {
            //Caso o arquivo nao exista, criar o header
            header novoHeader;
            novoHeader.root = -1;
            novoHeader.blockSize = this->blockSize;
            novoHeader.numBlocos = 1;

            // Zera o buffer antes de usar (boa prática)
            memset(buffer, 0, this->blockSize);
            // Copia o novo header para o buffer
            memcpy(buffer, &novoHeader, sizeof(header));
            // Escreve o header no arquivo.
            blockManager.writeBlock(0, buffer);
        }

        // Calcula 'm' depois de tudo estar definido.
        int overhead = sizeof(bool) + sizeof(int) + 2 * sizeof(long);
        m = floor((this->blockSize - overhead) / (sizeof(int) + sizeof(long)));

    } catch (const std::exception& e) {
        // precisamos garantir que a memória seja liberada antes de o programa parar.
        delete[] buffer;
        // Re-lança a exceção para que o programa que chamou o construtor saiba que algo deu errado.
        throw; 
    }
    delete[] buffer;
}

// Converte um objeto Node para um buffer de bytes
void BPlusTreeInt::serializeNode(const BPlusTreeInt::Node& node, char* buffer) {
    // 1. Copia os campos de tamanho fixo
    char* ptr = buffer; // Ponteiro para a posição atual no buffer

    memcpy(ptr, &node.isLeaf, sizeof(bool));
    ptr += sizeof(bool);

    memcpy(ptr, &node.numKeys, sizeof(int));
    ptr += sizeof(int);

    memcpy(ptr, &node.next, sizeof(long));
    ptr += sizeof(long);

    // 2. Copia o conteúdo do vector de chaves
    memcpy(ptr, node.keys.data(), node.numKeys * sizeof(int));
    ptr += node.numKeys * sizeof(int);

    // 3. Se não for folha, copia o conteúdo do vector de filhos
    if (!node.isLeaf) {
        memcpy(ptr, node.childrenOrPointers.data(), (node.numKeys + 1) * sizeof(long));
    }
}

// Converte um buffer de bytes de volta para um objeto Node
void BPlusTreeInt::deserializeNode(const char* buffer, BPlusTreeInt::Node& node) {
    const char* ptr = buffer; // Ponteiro para a posição atual no buffer

    // 1. Lê os campos de tamanho fixo
    memcpy(&node.isLeaf, ptr, sizeof(bool));
    ptr += sizeof(bool);

    memcpy(&node.numKeys, ptr, sizeof(int));
    ptr += sizeof(int);

    memcpy(&node.next, ptr, sizeof(long));
    ptr += sizeof(long);
    
    // 2. Lê as chaves
    node.keys.resize(node.numKeys); // IMPORTANTE: Alocar espaço no vector
    memcpy(node.keys.data(), ptr, node.numKeys * sizeof(int));
    ptr += node.numKeys * sizeof(int);

    // 3. Se não for folha, lê os filhos
    if (!node.isLeaf) {
        node.childrenOrPointers.resize(node.numKeys + 1); // IMPORTANTE: Alocar espaço
        memcpy(node.childrenOrPointers.data(), ptr, (node.numKeys + 1) * sizeof(long));
    }
}

void BPlusTreeInt::writeNodeToDisk(BPlusTreeInt::Node* node) {
    char* buffer = new char[blockSize];
    serializeNode(*node, buffer);
    blockManager.writeBlock(node->selfBlock, buffer);
    delete[] buffer;
}

BPlusTreeInt::Node* BPlusTreeInt::readNodeFromDisk(long blockNum, BPlusTreeInt::Node* node) {
    char* buffer = new char[blockSize];
    blockManager.readBlock(blockNum, buffer);
    deserializeNode(buffer, *node);
    delete[] buffer;
    return node;
}


/**
 * @brief (splitChild) Essa é uma das mais importantes.
 *
 * Lógica: Quando um nó filho ('child') tá lotado (2*t - 1 chaves)
 * e a gente tenta inserir mais, essa função é chamada.
**/ 
void BPlusTreeInt::splitChild(Node* parent, int index,Node* child){
    // [Sua Nota] Cria um novo nó "irmão", com o mesmo status (folha ou não)
    // do nó que será dividido.
    Node* newChild = new Node(child->isLeaf);

    // [Sua Nota] Insere o 'newChild' no vetor de filhos do 'parent',
    // na posição 'index + 1' (logo ao lado do 'child' original).
    parent->children.insert(
        parent->children.begin() + index + 1, newChild);

    // [Sua Nota] "Promove" a chave do meio (m-1) do 'child' lotado
    // para o 'parent', na posição 'index'.
    parent->keys.insert(parent->keys.begin() + index,
                        child->keys[m - 1]);

    // [Sua Nota] Copia a "metade direita" das chaves do 'child'
    // original para o 'newChild'.
    newChild->keys.assign(child->keys.begin() + m,
                          child->keys.end());

    // [Sua Nota] Reduz o tamanho do 'child' original para que ele
    // contenha apenas a "metade esquerda" das chaves.
    child->keys.resize(m - 1);

    // [Sua Nota] Se o 'child' NÃO for uma folha (for um nó interno),
    // temos que dividir também o vetor de 'children' dele.
    if (!child->isLeaf) {
        // [Sua Nota] O 'newChild' fica com a "metade direita" dos filhos.
        newChild->children.assign(child->children.begin() + m,
                                  child->children.end());
        // [Sua Nota] O 'child' original fica com a "metade esquerda" dos filhos.
        child->children.resize(m);
    }

    // Se o 'child' FOR uma folha, precisamos atualizar a "lista ligada"
    // de folhas (a corrente 🔗 que falamos).
    if (child->isLeaf) {
        // O 'newChild' aponta para quem o 'child' apontava antes.
        newChild->next = child->next;
        // O 'child' agora aponta para o 'newChild'.
        // Ordem: child -> newChild -> (o que era o next do child)
        child->next = newChild;
    }
}

/**
 * @brief Insere uma chave em um nó que *tem certeza* que não está cheio.
 * [Sua Nota] Esta é a função "trabalhadora" recursiva. A 'insert' pública
 * chama esta, e esta função "desce" pela árvore até encontrar o local certo.
 *
 * @param node O nó atual que estamos inspecionando (começa com a raiz).
 * @param key A chave (do tipo genérico int) que queremos inserir.
**/
void BPlusTreeInt::insertNonFull(Node* node, int key)
{
    // ==========================================================
    // CASO BASE (Recursão para aqui)
    // ==========================================================
    // [Sua Nota] Se o nó atual é uma folha, este é o fim da linha.
    // É aqui que a chave deve ser realmente inserida.
    if (node->isLeaf) {
        
        // [Sua Nota] Encontra a posição correta para inserir a chave
        // mantendo o vetor 'keys' ordenado.
        //
        // std::upper_bound: Encontra o primeiro elemento que é
        // *maior* que 'key'. Inserir a 'key' *antes* dele
        // mantém a ordem perfeitamente.
        node->keys.insert(
            upper_bound(node->keys.begin(), node->keys.end(), key),
            key
        );
    }
    // ==========================================================
    // CASO RECURSIVO (Nó interno)
    // ==========================================================
    // [Sua Nota] Se não é uma folha, é um nó interno (um "guia").
    // Precisamos decidir para qual "porta" (filho) devemos descer.
    else {
        // [Sua Nota] Procura o índice do filho correto, começando
        // da direita para a esquerda.
        int i = node->keys.size() - 1;
        while (i >= 0 && key < node->keys[i]) {
            i--;
        }
        
        // [Sua Nota] O loop parou na chave *antes* do caminho que
        // queremos. O caminho correto é 'i + 1'.
        i++; 

        // ==========================================================
        // A "MÁGICA" DA B+ intREE: Dividir antes de descer
        // ==========================================================
        // [Sua Nota] Verificamos se a "gaveta" (filho) para onde
        // vamos descer está LOintADA. (2*m - 1 é o n° máximo de chaves).
        if (node->children[i]->keys.size() == 2 * m - 1) {
            
            // [Sua Nota] Se estiver lotada, chamamos 'splitChild'
            // para dividi-la AGORA, *antes* de descermos.
            splitChild(node, i, node->children[i]);

            // [Sua Nota] Após a divisão, uma chave do filho subiu
            // para este nó ('node') na posição 'i'.
            // Precisamos checar se a 'key' que queremos inserir
            // é maior que essa chave que acabou de subir.
            // Se for, o caminho certo agora é o *novo* irmão (i + 1).
            if (key > node->keys[i]) {
                i++;
            }
        }
        
        // [Sua Nota] Agora temos CERintEZA que o filho 'children[i]'
        // tem espaço. Chamamos a mesma função de novo, mas
        // um nível abaixo (no filho que escolhemos).
        insertNonFull(node->children[i], key);
    }
}

/**
 * @brief (search) A mais fácil.
 *
 * Lógica: É só um 'while' que desce a árvore. Em cada nó,
 * ela acha o caminho certo ('i') pra descer.
 * Se em algum momento 'key == current->keys[i]', achou (true).
 * Se chegar na folha ('isLeaf') e não achar, não existe (false).
**/
template <typename T> bool BPlusTree<T>::search(T key)
{
    Node* current = root;
    while (current != nullptr) {
        // Acha o primeiro 'i' onde key <= keys[i]
        int i = 0;
        while (i < current->keys.size()
               && key > current->keys[i]) {
            i++;
        }
        // Achou!
        if (i < current->keys.size()
            && key == current->keys[i]) {
            return true;
        }
        // Chegou na folha e não achou. Fim.
        if (current->isLeaf) {
            return false;
        }
        // Desce pro filho certo
        current = current->children[i];
    }
    // 'root' era nula.
    return false;
}


/**
 * @brief (insert - public) A que o usuário chama.
 *
 * Lógica: Lida com os dois casos chatos da raiz.
**/
template <typename T>
void BPlusTree<T>::insert(T key)
{
    // CASO 1: Árvore vazia. Eu só crio a 'root' como folha
    // e coloco a chave.
    if () {
        root = new Node(true);
        root->keys.push_back(key);
    }
    // CASO 2: Árvore já existe.
    else {
        // Sub-caso: A *raiz* tá lotada.
        // A raiz não tem pai, então eu não posso só chamar 'splitChild'.
        if (root->keys.size() == 2 * t - 1) {
            // 1. Crio um 'newRoot' (que não é folha).
            Node* newRoot = new Node();
            // 2. Faço o 'root' antigo virar filho ('children[0]') do 'newRoot'.
            newRoot->children.push_back(root);
            // 3. Chamo 'splitChild' pra dividir o 'root' antigo,
            //    subindo a chave do meio pro 'newRoot'.
            splitChild(newRoot, 0, root);
            // 4. Faço o 'root' da árvore apontar pro 'newRoot'.
            //    A árvore cresceu 1 nível.
            root = newRoot;
        }
        // Depois de resolver esses casos, eu chamo a 'insertNonFull'
        // pra fazer o trabalho de verdade.
        insertNonFull(root, key);
    }
}
