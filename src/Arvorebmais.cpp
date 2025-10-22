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
      positionRoot(-1)
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
            this->positionRoot = hdr.positionRoot;

        } else {
            //Caso o arquivo nao exista, criar o header
            header novoHeader;
            novoHeader.positionRoot = -1;
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

// Converte um objeto Node para um buffer de bytes (CORRIGIDO)
void BPlusTreeInt::serializeNode(const BPlusTreeInt::Node& node, char* buffer) {
    char* ptr = buffer; 

    // 1. Copia os campos de tamanho fixo (Isto estava correto)
    memcpy(ptr, &node.isLeaf, sizeof(bool));
    ptr += sizeof(bool);
    memcpy(ptr, &node.numKeys, sizeof(int));
    ptr += sizeof(int);
    memcpy(ptr, &node.next, sizeof(long));
    ptr += sizeof(long);

    // 2. Copia o conteúdo do vector de chaves (Isto estava correto)
    memcpy(ptr, node.keys.data(), node.numKeys * sizeof(int));
    ptr += node.numKeys * sizeof(int);

    // 3. Copia os ponteiros (de dados OU de filhos)
    if (node.isLeaf) {
        // Se é folha, salva 'numKeys' ponteiros de DADOS
        memcpy(ptr, node.childrenOrPointers.data(), node.numKeys * sizeof(long));
        // (Opcional) Mover o ptr: ptr += node.numKeys * sizeof(long);
    } else {
        // Se é interno, salva 'numKeys + 1' ponteiros de FILHOS
        memcpy(ptr, node.childrenOrPointers.data(), (node.numKeys + 1) * sizeof(long));
        // (Opcional) Mover o ptr: ptr += (node.numKeys + 1) * sizeof(long);
    }
}

// Converte um buffer de bytes de volta para um objeto Node (CORRIGIDO)
void BPlusTreeInt::deserializeNode(const char* buffer, BPlusTreeInt::Node& node) {
    const char* ptr = buffer; 

    // 1. Lê os campos de tamanho fixo (Isto estava correto)
    memcpy(&node.isLeaf, ptr, sizeof(bool));
    ptr += sizeof(bool);
    memcpy(&node.numKeys, ptr, sizeof(int));
    ptr += sizeof(int);
    memcpy(&node.next, ptr, sizeof(long));
    ptr += sizeof(long);
    
    // 2. Lê as chaves (Isto estava correto)
    node.keys.resize(node.numKeys); 
    memcpy(node.keys.data(), ptr, node.numKeys * sizeof(int));
    ptr += node.numKeys * sizeof(int);

    // 3. Lê os ponteiros (de dados OU de filhos)
    if (node.isLeaf) {
        // Se é folha, lê 'numKeys' ponteiros de DADOS
        node.childrenOrPointers.resize(node.numKeys); 
        memcpy(node.childrenOrPointers.data(), ptr, node.numKeys * sizeof(long));
    } else {
        // Se é interno, lê 'numKeys + 1' ponteiros de FILHOS
        node.childrenOrPointers.resize(node.numKeys + 1); 
        memcpy(node.childrenOrPointers.data(), ptr, (node.numKeys + 1) * sizeof(long));
    }
}

// Escreve um nó no disco
void BPlusTreeInt::writeNodeToDisk(BPlusTreeInt::Node* node) {
    char* buffer = new char[blockSize];
    serializeNode(*node, buffer);
    blockManager.writeBlock(node->selfPosition, buffer);
    delete[] buffer;
}

// Lê um nó do disco e o desserializa
BPlusTreeInt::Node* BPlusTreeInt::readNodeFromDisk(long blockNum, BPlusTreeInt::Node* node) {
    char* buffer = new char[blockSize];
    blockManager.readBlock(blockNum, buffer);
    deserializeNode(buffer, *node);
    node->selfPosition = blockNum;
    delete[] buffer;
    return node;
}

//  Escreve o header da árvore B+ no bloco 0 do arquivo de índice.
void BPlusTreeInt::writeHeader() {
    // 1. Cria e preenche a struct com os dados atuais.
    header hdr;
    hdr.positionRoot = this->positionRoot;
    hdr.blockSize = this->blockSize;
    hdr.numBlocos = blockManager.getBlockCount();

    // 2. Aloca um buffer na heap.
    char* buffer = new char[blockSize];
    
    // 3. (Opcional, mas recomendado) Zera o buffer para evitar escrever lixo.
    memset(buffer, 0, blockSize);

    // 4. Copia os bytes da struct para o início do buffer.
    memcpy(buffer, &hdr, sizeof(header));

    // 5. Escreve o buffer no bloco 0.
    blockManager.writeBlock(0, buffer);

    // 6. Libera a memória alocada.
    delete[] buffer;
}

// Lê o header da árvore B+ do bloco 0 do arquivo de índice.
void BPlusTreeInt::readHeader() {
    // 1. Aloca um buffer na heap para receber os dados.
    char* buffer = new char[blockSize];

    // 2. Lê o bloco 0 do disco para o buffer.
    blockManager.readBlock(0, buffer);

    // 3. Cria uma struct para receber a cópia.
    header hdr;
    
    // 4. Copia os bytes do buffer para a struct.
    memcpy(&hdr, buffer, sizeof(header));
    
    // 5. Libera a memória, pois os dados já estão seguros em 'hdr'.
    delete[] buffer;

    // 6. Atualiza os atributos da classe.
    this->positionRoot = hdr.positionRoot;
    this->blockSize = hdr.blockSize;
}



/**
 * @brief (splitChild) REESCRITO PARA DISCO
 *
 * @param parent O nó pai (JÁ EM MEMÓRIA) que será modificado.
 * @param childIndex O índice (em parent->childrenOrPointers) do filho que está lotado.
**/ 
void BPlusTreeInt::splitChild(Node* parent, int childIndex){
    
    // 1. Cria o novo "irmão" (newSibling) e aloca um bloco para ele no disco
    Node* newSibling = new Node();
    newSibling->selfPosition = blockManager.allocateBlock();

    // 2. Carrega o filho lotado ('child') do disco
    long childOffset = parent->childrenOrPointers[childIndex];
    Node* child = new Node();
    readNodeFromDisk(childOffset, child);
    
    // 3. Define o status do novo irmão (folha ou não)
    newSibling->isLeaf = child->isLeaf;

    // 4. Calcula o "ponto do meio" para o split
    int middleIndex;
    int keyToPromote; // A chave que vai subir para o 'parent'

    if (child->isLeaf) {
        // --- Split de NÓ FOLHA ---
        // 'm' é o n° max de pares. Um nó folha lotado tem 'm' chaves.
        // Ponto do meio: m / 2.
        middleIndex = m / 2;
        
        // A chave no 'middleIndex' é a primeira a ir para o novo irmão.
        // Em uma B+ Tree, a chave promovida de uma folha é COPIADA, não movida.
        keyToPromote = child->keys[middleIndex]; 

        // Copia a "metade direita" (chaves) para o novo irmão
        newSibling->keys.assign(child->keys.begin() + middleIndex, child->keys.end());
        // Copia a "metade direita" (ponteiros de DADOS)
        newSibling->childrenOrPointers.assign(child->childrenOrPointers.begin() + middleIndex, child->childrenOrPointers.end());

        // Atualiza os contadores de chaves
        newSibling->numKeys = m - middleIndex;
        child->numKeys = middleIndex;
        
        // Atualiza os vetores do filho antigo (trunca)
        child->keys.resize(middleIndex);
        child->childrenOrPointers.resize(middleIndex);

        // Atualiza a lista ligada de folhas
        newSibling->next = child->next;
        child->next = newSibling->selfPosition;

    } else {
        // --- Split de NÓ INTERNO ---
        // Um nó interno lotado tem 'm-1' chaves (e 'm' filhos).
        // Ponto do meio: (m-1) / 2
        middleIndex = (m - 1) / 2;

        // A chave do meio é MOVIDA para o pai.
        keyToPromote = child->keys[middleIndex];

        // Copia a "metade direita" (chaves) para o novo irmão
        // (sem incluir a chave promovida)
        newSibling->keys.assign(child->keys.begin() + middleIndex + 1, child->keys.end());
        // Copia a "metade direita" (ponteiros de FILHOS)
        newSibling->childrenOrPointers.assign(child->childrenOrPointers.begin() + middleIndex + 1, child->childrenOrPointers.end());
        
        // Atualiza os contadores de chaves
        newSibling->numKeys = (m - 1) - (middleIndex + 1);
        child->numKeys = middleIndex;

        // Atualiza os vetores do filho antigo (trunca)
        child->keys.resize(middleIndex);
        child->childrenOrPointers.resize(middleIndex + 1); // Nós internos têm numKeys+1 filhos
    }

    // 5. ATUALIZA O NÓ PAI (que já estava em memória)
    // Insere a chave promovida
    parent->keys.insert(parent->keys.begin() + childIndex, keyToPromote);
    // Insere o ponteiro para o novo irmão
    parent->childrenOrPointers.insert(parent->childrenOrPointers.begin() + childIndex + 1, newSibling->selfPosition);
    parent->numKeys++;

    // 6. ESCREVE OS 3 NÓS MODIFICADOS DE VOLTA NO DISCO
    writeNodeToDisk(parent);
    writeNodeToDisk(child);
    writeNodeToDisk(newSibling);

    // 7. Limpa a memória
    delete child;
    delete newSibling;
}

/**
 * @brief Insere (key, dataPointer) em um nó que *não* está cheio.
 * REESCRITO PARA DISCO
 *
 * @param node O nó atual (JÁ EM MEMÓRIA) que estamos inspecionando.
 * @param key A chave que queremos inserir.
 * @param dataPointer O ponteiro de dado associado.
**/
void BPlusTreeInt::insertNonFull(Node* node, int key, long dataPointer)
{
    // ==========================================================
    // CASO BASE (Recursão para aqui)
    // ==========================================================
    if (node->isLeaf) {
        // Nó é folha. Encontra a posição correta e insere.
        
        // std::lower_bound: Encontra o primeiro elemento que é >= key.
        auto it = std::lower_bound(node->keys.begin(), node->keys.end(), key);
        int i = std::distance(node->keys.begin(), it);

        // Insere a chave
        node->keys.insert(node->keys.begin() + i, key);
        // Insere o ponteiro de DADO
        node->childrenOrPointers.insert(node->childrenOrPointers.begin() + i, dataPointer);
        node->numKeys++;
        
        // Escreve o nó modificado de volta no disco
        writeNodeToDisk(node);
    }
    // ==========================================================
    // CASO RECURSIVO (Nó interno)
    // ==========================================================
    else {
        // Nó é interno. Encontra o filho correto para descer.
        
        // Procura o índice do filho correto,
        // Encontra a primeira chave > key
        auto it = std::upper_bound(node->keys.begin(), node->keys.end(), key);
        int i = std::distance(node->keys.begin(), it);
        
        // 'i' é o índice do ponteiro do filho para onde devemos descer.

        // ==========================================================
        // A "MÁGICA" DA B+ TREE: Dividir antes de descer
        // ==========================================================

        // 1. Carrega o filho 'i' do disco para checar se está lotado
        long childOffset = node->childrenOrPointers[i];
        Node* childNode = new Node();
        readNodeFromDisk(childOffset, childNode);

        // 2. Define a capacidade máxima do filho
        bool childIsFull = false;
        if (childNode->isLeaf) {
            if (childNode->numKeys == m) { // Folha lota com 'm'
                childIsFull = true;
            }
        } else {
            if (childNode->numKeys == m - 1) { // Interno lota com 'm-1'
                childIsFull = true;
            }
        }

        // 3. Se o filho estiver lotado, divide ele AGORA.
        if (childIsFull) {
            // 'node' é o pai (em memória), 'i' é o índice do filho
            splitChild(node, i); 
            
            // O split ALTEROU o 'node' (pai).
            // Precisamos checar se a 'key' agora pertence
            // ao novo irmão (que está em 'i+1').
            if (key > node->keys[i]) {
                i++;
            }
        
            // O 'childNode' que tínhamos carregado está obsoleto/foi deletado
            // pelo splitChild. Recarregamos o nó filho correto (agora com espaço).
            childOffset = node->childrenOrPointers[i];
            readNodeFromDisk(childOffset, childNode);
            
        } 
        
        // 4. Agora temos CERTEZA que o 'childNode' (filho[i]) tem espaço.
        // Chamamos a recursão para descer um nível.
        insertNonFull(childNode, key, dataPointer);
        
        // 5. Limpa a memória
        delete childNode;
    }
}

/**
 * @brief (insert - public) A que o usuário chama.
 * TRADUZIDO PARA DISCO
**/ 
void BPlusTreeInt::insert(int key, long dataPointer)
{
    // CASO 1: Árvore vazia. Eu só crio a 'root' como folha
    // e coloco a chave.
    if (positionRoot == -1) {
        Node* rootNode = new Node(true); // Cria a raiz, que também é uma folha.
        rootNode->selfPosition = blockManager.allocateBlock(); 
        this->positionRoot = rootNode->selfPosition; 

        rootNode->keys.push_back(key);
        rootNode->childrenOrPointers.push_back(dataPointer);
        rootNode->numKeys = 1;

        writeNodeToDisk(rootNode); 
        writeHeader(); 
        delete rootNode; 
        return;
    }
    // CASO 2: Árvore já existe.
    else {
        // 1. Carrega o nó raiz ATUAL do disco para a memória.
        Node* rootNode = new Node();
        readNodeFromDisk(this->positionRoot, rootNode);

        // 2. Verifica se a raiz está lotada
        bool rootIsFull = false;
        if (rootNode->isLeaf) {
            if (rootNode->numKeys == m) { // Folha lota com 'm'
                rootIsFull = true;
            }
        } else {
            if (rootNode->numKeys == m - 1) { // Interno lota com 'm-1'
                rootIsFull = true;
            }
        }

        // Sub-caso: A *raiz* tá lotada.
        // A raiz não tem pai, então eu não posso só chamar 'splitChild'.
        if (rootIsFull) {
            // A. Cria uma NOVA raiz (na memória)
            Node* newRootNode = new Node(false); // Nova raiz NUNCA é folha (porque vai ter filhos)
            newRootNode->selfPosition = blockManager.allocateBlock(); 

            // B. O primeiro filho da nova raiz é a RAIZ ANTIGA (a posição dela)
            newRootNode->childrenOrPointers.push_back(this->positionRoot); 
            // (numKeys da newRootNode ainda é 0)

            // C. ATUALIZA a raiz da árvore (na classe e no header)
            this->positionRoot = newRootNode->selfPosition;
            writeHeader();

            // D. Chama splitChild. 
            // Pai = newRootNode (em memória), Índice do filho lotado = 0
            splitChild(newRootNode, 0); 
            
            // E. Agora, o newRootNode tem 1 chave.
            // Precisamos decidir para qual filho descer
            insertNonFull(newRootNode, key, dataPointer);

            // F. Limpa a memória
            delete newRootNode;
        } 
        // 4. Sub-caso: A raiz NÃO está lotada.
        else {
            // A raiz tem espaço. Só chama a inserção nela.
            insertNonFull(rootNode, key, dataPointer);
        }
        
        // Limpa o rootNode que carregamos no início
        delete rootNode; 
    }
}

/**
 * @brief (search) A mais fácil.
 *
 * Lógica: É só um 'while' que desce a árvore. Em cada nó,
 * ela acha o caminho certo ('i') pra descer.
 * Se em algum momento 'key == current->keys[i]', achou (true).
 * Se chegar na folha ('isLeaf') e não achar, não existe (false).
 */
long BPlusTreeInt::search(int key){
    if (positionRoot == -1) {
        return -1; // Árvore vazia
    }

    Node* current = new Node();
    readNodeFromDisk(positionRoot, current);

    while (true) {
        
        // Encontra o primeiro índice 'i' onde key <= current->keys[i]
        int i = 0;
        while (i < current->numKeys && key > current->keys[i]) {
            i++;
        }
        // Agora, 'i' é o índice do primeiro elemento >= key

        if (current->isLeaf) {
            // --- Está na folha ---
            // Verifica se a chave no índice 'i' é a que procuramos
            if (i < current->numKeys && current->keys[i] == key) {
                // Achou! Retorna o ponteiro de dado.
                long dataPtr = current->childrenOrPointers[i];
                delete current;
                return dataPtr;
            } else {
                // Não achou.
                delete current;
                return -1;
            }
        } else {
            // --- É nó interno ---
            
            // Se a chave for EXATAMENTE igual a uma chave interna, na B+ Tree
            // o valor está na sub-árvore da DIREITA daquela chave.
            if (i < current->numKeys && current->keys[i] == key) {
                 i++;
            }
            
            // 'i' é o índice do filho para onde descer.
            long childPosition = current->childrenOrPointers[i];
            
            // Reutiliza o ponteiro 'current' para carregar o filho.
            readNodeFromDisk(childPosition, current); 
        }
    }
}