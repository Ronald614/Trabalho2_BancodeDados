// Bibliotecas padrão necessárias
#include <algorithm> // Para funções como std::lower_bound (útil na busca)
#include <iostream>  // Para std::cout (impressão na tela)
#include <vector>    // Para std::vector (nossas listas dinâmicas)
#include <cstring>   // Para memcpy
#include <math.h>    // Para floor (na definição de 'm')
#include "GerenciaBlocos.hpp"// Gerenciamento de blocos em disco
#include "Arvorebmais.hpp"  // Declaração da classe BPlusTreeInt

/*
 * ==========================================================
 * Implementação dos Métodos da Classe BPlusTreeInt
 * ==========================================================
 */

// Constructor
BPlusTreeInt::BPlusTreeInt(const std::string& nomeArquivo, const size_t tamanhoBloco_arg)
    : tamanhoBloco(static_cast<int>(tamanhoBloco_arg)),
      nomeArquivo(nomeArquivo),
      gerenciador(nomeArquivo, tamanhoBloco_arg),
      idRaiz(-1)
{
    char* buffer = new char[this->tamanhoBloco];
    try { 
        //se a arvore existe o arquivo tem algo
        if (gerenciador.getTamanhoArquivo() > 0){

            gerenciador.lerBloco(0, buffer);
            cabecalho hdr;
            memcpy(&hdr, buffer, sizeof(cabecalho));
    
            // Verifica se o tamanho do bloco no arquivo bate com o fornecido
            if (hdr.tamanhoBloco != this->tamanhoBloco) {
                delete[] buffer; 
                throw std::runtime_error("Erro: O tamanho do bloco fornecido é inconsistente com o do arquivo!");
            }
            this->idRaiz = hdr.idRaiz;

        } else {
            //Caso o arquivo nao exista, criar o cabecalho
            cabecalho novocabecalho;
            novocabecalho.idRaiz = -1;
            novocabecalho.tamanhoBloco = this->tamanhoBloco;
            novocabecalho.numBlocos = 1;

            // Zera o buffer antes de usar (boa prática)
            memset(buffer, 0, this->tamanhoBloco);
            // Copia o novo cabecalho para o buffer
            memcpy(buffer, &novocabecalho, sizeof(cabecalho));
            // Escreve o cabecalho no arquivo.
            gerenciador.escreveBloco(0, buffer);
        }

        // Calcula 'm' depois de tudo estar definido.
        int tamMetadados = sizeof(bool) + sizeof(int) + sizeof(long) + sizeof(long);
        m = floor((this->tamanhoBloco - tamMetadados) / (sizeof(int) + sizeof(long)));

    } catch (const std::exception& e) {
        // precisamos garantir que a memória seja liberada antes de o programa parar.
        delete[] buffer;
        // Re-lança a exceção para que o programa que chamou o construtor saiba que algo deu errado.
        throw; 
    }
    delete[] buffer;
}

// Converte um objeto No para um buffer de bytes (CORRIGIDO)
void serializaNo(const No& No, char* buffer) {
    char* ptr = buffer; 

    // 1. Copia os campos de tamanho fixo (Isto estava correto)
    memcpy(ptr, &No.ehFolha, sizeof(bool));
    ptr += sizeof(bool);
    memcpy(ptr, &No.numChaves, sizeof(int));
    ptr += sizeof(int);
    memcpy(ptr, &No.proximo, sizeof(long));
    ptr += sizeof(long);

    // 2. Copia o conteúdo do vector de chaves (Isto estava correto)
    memcpy(ptr, No.vetorChaves.data(), No.numChaves * sizeof(int));
    ptr += No.numChaves * sizeof(int);

    // 3. Copia os ponteiros (de dados OU de filhos)
    if (No.ehFolha) {
        // Se é folha, salva 'numChaves' ponteiros de DADOS
        memcpy(ptr, No.vetorApontadores.data(), No.numChaves * sizeof(long));
        // (Opcional) Mover o ptr: ptr += No.numChaves * sizeof(long);
    } else {
        // Se é interno, salva 'numChaves + 1' ponteiros de FILHOS
        memcpy(ptr, No.vetorApontadores.data(), (No.numChaves + 1) * sizeof(long));
        // (Opcional) Mover o ptr: ptr += (No.numChaves + 1) * sizeof(long);
    }
}

// Converte um buffer de bytes de volta para um objeto No (CORRIGIDO)
void BPlusTreeInt::deserializaNo(const char* buffer, No& No) {
    const char* ptr = buffer; 

    // 1. Lê os campos de tamanho fixo (Isto estava correto)
    memcpy(&No.ehFolha, ptr, sizeof(bool));
    ptr += sizeof(bool);
    memcpy(&No.numChaves, ptr, sizeof(int));
    ptr += sizeof(int);
    memcpy(&No.proximo, ptr, sizeof(long));
    ptr += sizeof(long);
    
    // 2. Lê as chaves (Isto estava correto)
    No.vetorChaves.resize(No.numChaves); 
    memcpy(No.vetorChaves.data(), ptr, No.numChaves * sizeof(int));
    ptr += No.numChaves * sizeof(int);

    // 3. Lê os ponteiros (de dados OU de filhos)
    if (No.ehFolha) {
        // Se é folha, lê 'numChaves' ponteiros de DADOS
        No.vetorApontadores.resize(No.numChaves); 
        memcpy(No.vetorApontadores.data(), ptr, No.numChaves * sizeof(long));
    } else {
        // Se é interno, lê 'numChaves + 1' ponteiros de FILHOS
        No.vetorApontadores.resize(No.numChaves + 1); 
        memcpy(No.vetorApontadores.data(), ptr, (No.numChaves + 1) * sizeof(long));
    }
}

// Escreve um nó no disco
void BPlusTreeInt::escreverNo(No* No) {
    char* buffer = new char[tamanhoBloco];
    serializaNo(*No, buffer);
    gerenciador.escreveBloco(No->selfId, buffer);
    delete[] buffer;
}

// Lê um nó do disco e o desserializa
No* BPlusTreeInt::lerNo(long blockNum, No* No) {
    char* buffer = new char[tamanhoBloco];
    gerenciador.lerBloco(blockNum, buffer);
    deserializaNo(buffer, *No);
    No->selfId = blockNum;
    delete[] buffer;
    return No;
}

//  Escreve o cabecalho da árvore B+ no bloco 0 do arquivo de índice.
void BPlusTreeInt::escreverCabecalho() {
    // 1. Cria e preenche a struct com os dados atuais.
    cabecalho hdr;
    hdr.idRaiz = this->idRaiz;
    hdr.tamanhoBloco = this->tamanhoBloco;
    hdr.numBlocos = gerenciador.getTamanhoArquivo()/this->tamanhoBloco;

    // 2. Aloca um buffer na heap.
    char* buffer = new char[tamanhoBloco];
    
    // 3. (Opcional, mas recomendado) Zera o buffer para evitar escrever lixo.
    memset(buffer, 0, tamanhoBloco);

    // 4. Copia os bytes da struct para o início do buffer.
    memcpy(buffer, &hdr, sizeof(cabecalho));

    // 5. Escreve o buffer no bloco 0.
    gerenciador.escreveBloco(0, buffer);

    // 6. Libera a memória alocada.
    delete[] buffer;
}

// Lê o cabecalho da árvore B+ do bloco 0 do arquivo de índice.
void BPlusTreeInt::lerCabecalho() {
    // 1. Aloca um buffer na heap para receber os dados.
    char* buffer = new char[tamanhoBloco];

    // 2. Lê o bloco 0 do disco para o buffer.
    gerenciador.lerBloco(0, buffer);

    // 3. Cria uma struct para receber a cópia.
    cabecalho hdr;
    
    // 4. Copia os bytes do buffer para a struct.
    memcpy(&hdr, buffer, sizeof(cabecalho));
    
    // 5. Libera a memória, pois os dados já estão seguros em 'hdr'.
    delete[] buffer;

    // 6. Atualiza os atributos da classe.
    this->idRaiz = hdr.idRaiz;
    this->tamanhoBloco = hdr.tamanhoBloco;
}

/**
 * @brief (splitChild) REESCRITO PARA DISCO
 *
 * @param parent O nó pai (JÁ EM MEMÓRIA) que será modificado.
 * @param childIndex O índice (em parent->vetorApontadores) do filho que está lotado.
**/ 
void BPlusTreeInt::splitChild(No* parent, int childIndex){
    
    // 1. Cria o novo "irmão" (newSibling) e aloca um bloco para ele no disco
    No* newSibling = new No();
    newSibling->selfId = gerenciador.retornaNovoId();

    // 2. Carrega o filho lotado ('child') do disco
    long childOffset = parent->vetorApontadores[childIndex];
    No* child = new No();
    lerNo(childOffset, child);
    
    // 3. Define o status do novo irmão (folha ou não)
    newSibling->ehFolha = child->ehFolha;

    // 4. Calcula o "ponto do meio" para o split
    int middleIndex;
    int keyToPromote; // A chave que vai subir para o 'parent'

    if (child->ehFolha) {
        // --- Split de NÓ FOLHA ---
        // 'm' é o n° max de pares. Um nó folha lotado tem 'm' chaves.
        // Ponto do meio: m / 2.
        middleIndex = m / 2;
        
        // A chave no 'middleIndex' é a primeira a ir para o novo irmão.
        // Em uma B+ Tree, a chave promovida de uma folha é COPIADA, não movida.
        keyToPromote = child->vetorChaves[middleIndex]; 

        // Copia a "metade direita" (chaves) para o novo irmão
        newSibling->vetorChaves.assign(child->vetorChaves.begin() + middleIndex, child->vetorChaves.end());
        // Copia a "metade direita" (ponteiros de DADOS)
        newSibling->vetorApontadores.assign(child->vetorApontadores.begin() + middleIndex, child->vetorApontadores.end());

        // Atualiza os contadores de chaves
        newSibling->numChaves = m - middleIndex;
        child->numChaves = middleIndex;
        
        // Atualiza os vetores do filho antigo (trunca)
        child->vetorChaves.resize(middleIndex);
        child->vetorApontadores.resize(middleIndex);

        // Atualiza a lista ligada de folhas
        newSibling->proximo = child->proximo;
        child->proximo = newSibling->selfId;

    } else {
        // --- Split de NÓ INTERNO ---
        // Um nó interno lotado tem 'm-1' chaves (e 'm' filhos).
        // Ponto do meio: (m-1) / 2
        middleIndex = (m - 1) / 2;

        // A chave do meio é MOVIDA para o pai.
        keyToPromote = child->vetorChaves[middleIndex];

        // Copia a "metade direita" (chaves) para o novo irmão
        // (sem incluir a chave promovida)
        newSibling->vetorChaves.assign(child->vetorChaves.begin() + middleIndex + 1, child->vetorChaves.end());
        // Copia a "metade direita" (ponteiros de FILHOS)
        newSibling->vetorApontadores.assign(child->vetorApontadores.begin() + middleIndex + 1, child->vetorApontadores.end());
        
        // Atualiza os contadores de chaves
        newSibling->numChaves = (m - 1) - (middleIndex + 1);
        child->numChaves = middleIndex;

        // Atualiza os vetores do filho antigo (trunca)
        child->vetorChaves.resize(middleIndex);
        child->vetorApontadores.resize(middleIndex + 1); // Nós internos têm numChaves+1 filhos
    }

    // 5. ATUALIZA O NÓ PAI (que já estava em memória)
    // Insere a chave promovida
    parent->vetorChaves.insert(parent->vetorChaves.begin() + childIndex, keyToPromote);
    // Insere o ponteiro para o novo irmão
    parent->vetorApontadores.insert(parent->vetorApontadores.begin() + childIndex + 1, newSibling->selfId);
    parent->numChaves++;

    // 6. ESCREVE OS 3 NÓS MODIFICADOS DE VOLTA NO DISCO
    escreverNo(parent);
    escreverNo(child);
    escreverNo(newSibling);

    // 7. Limpa a memória
    delete child;
    delete newSibling;
}

/**
 * @brief Insere (key, dataPointer) em um nó que *não* está cheio.
 * REESCRITO PARA DISCO
 *
 * @param No O nó atual (JÁ EM MEMÓRIA) que estamos inspecionando.
 * @param key A chave que queremos inserir.
 * @param dataPointer O ponteiro de dado associado.
**/
void BPlusTreeInt::insertNonFull(No* novo_no, int key, long dataPointer)
{
    // ==========================================================
    // CASO BASE (Recursão para aqui)
    // ==========================================================
    if (novo_no->ehFolha) {
        // Nó é folha. Encontra a posição correta e insere.
        
        // std::lower_bound: Encontra o primeiro elemento que é >= key.
        auto it = std::lower_bound(novo_no->vetorChaves.begin(), novo_no->vetorChaves.end(), key);
        int i = std::distance(novo_no->vetorChaves.begin(), it);

        // Insere a chave
        novo_no->vetorChaves.insert(novo_no->vetorChaves.begin() + i, key);
        // Insere o ponteiro de DADO
        novo_no->vetorApontadores.insert(novo_no->vetorApontadores.begin() + i, dataPointer);
        novo_no->numChaves++;
        
        // Escreve o nó modificado de volta no disco
        escreverNo(novo_no);
    }
    // ==========================================================
    // CASO RECURSIVO (Nó interno)
    // ==========================================================
    else {
        // Nó é interno. Encontra o filho correto para descer.
        
        // Procura o índice do filho correto,
        // Encontra a primeira chave > key
        auto it = std::upper_bound(novo_no->vetorChaves.begin(), novo_no->vetorChaves.end(), key);
        int i = std::distance(novo_no->vetorChaves.begin(), it);
        
        // 'i' é o índice do ponteiro do filho para onde devemos descer.

        // ==========================================================
        // A "MÁGICA" DA B+ TREE: Dividir antes de descer
        // ==========================================================

        // 1. Carrega o filho 'i' do disco para checar se está lotado
        long childOffset = novo_no->vetorApontadores[i];
        No* childNo = new No();
        lerNo(childOffset, childNo);

        // 2. Define a capacidade máxima do filho
        bool childIsFull = false;   
        if (childNo->ehFolha) {
            if (childNo->numChaves == m) { // Folha lota com 'm'
                childIsFull = true;
            }
        } else {
            if (childNo->numChaves == m - 1) { // Interno lota com 'm-1'
                childIsFull = true;
            }
        }

        // 3. Se o filho estiver lotado, divide ele AGORA.
        if (childIsFull) {
            // 'No' é o pai (em memória), 'i' é o índice do filho
            splitChild(novo_no, i); 
            
            // O split ALTEROU o 'No' (pai).
            // Precisamos checar se a 'key' agora pertence
            // ao novo irmão (que está em 'i+1').
            if (key > novo_no->vetorChaves[i]) {
                i++;
            }
        
            // O 'childNo' que tínhamos carregado está obsoleto/foi deletado
            // pelo splitChild. Recarregamos o nó filho correto (agora com espaço).
            childOffset = novo_no->vetorApontadores[i];
            lerNo(childOffset, childNo);
            
        } 
        
        // 4. Agora temos CERTEZA que o 'childNo' (filho[i]) tem espaço.
        // Chamamos a recursão para descer um nível.
        insertNonFull(childNo, key, dataPointer);
        
        // 5. Limpa a memória
        delete childNo;
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
    if (idRaiz == -1) {
        No* rootNo = new No(true); // Cria a raiz, que também é uma folha.
        rootNo->selfId = gerenciador.retornaNovoId(); 
        this->idRaiz = rootNo->selfId; // id dele vai ser a posição onde ele tá no arq

        rootNo->vetorChaves.push_back(key);
        rootNo->vetorApontadores.push_back(dataPointer);
        rootNo->numChaves = 1;

        escreverNo(rootNo); 
        escreverCabecalho(); 
        delete rootNo; 
        return;
    }
    // CASO 2: Árvore já existe.
    else {
        // 1. Carrega o nó raiz ATUAL do disco para a memória.
        No* rootNo = new No();
        lerNo(this->idRaiz, rootNo);

        // 2. Verifica se a raiz está lotada
        bool rootIsFull = false;
        if (rootNo->ehFolha) {
            if (rootNo->numChaves == m) { // Folha lota com 'm'
                rootIsFull = true;
            }
        } else {
            if (rootNo->numChaves == m - 1) { // Interno lota com 'm-1'
                rootIsFull = true;
            }
        }

        // Sub-caso: A *raiz* tá lotada.
        // A raiz não tem pai, então eu não posso só chamar 'splitChild'.
        if (rootIsFull) {
            // A. Cria uma NOVA raiz (na memória)
            No* newRootNo = new No(false); // Nova raiz NUNCA é folha (porque vai ter filhos)
            newRootNo->selfId = gerenciador.retornaNovoId(); 

            // B. O primeiro filho da nova raiz é a RAIZ ANTIGA (a posição dela)
            newRootNo->vetorApontadores.push_back(this->idRaiz); 
            // (numChaves da newRootNo ainda é 0)

            // C. ATUALIZA a raiz da árvore (na classe e no cabecalho)
            this->idRaiz = newRootNo->selfId;
            escreverCabecalho();

            // D. Chama splitChild. 
            // Pai = newRootNo (em memória), Índice do filho lotado = 0
            splitChild(newRootNo, 0); 
            
            // E. Agora, o newRootNo tem 1 chave.
            // Precisamos decidir para qual filho descer
            insertNonFull(newRootNo, key, dataPointer);

            // F. Limpa a memória
            delete newRootNo;
        } 
        // 4. Sub-caso: A raiz NÃO está lotada.
        else {
            // A raiz tem espaço. Só chama a inserção nela.
            insertNonFull(rootNo, key, dataPointer);
        }
        
        // Limpa o rootNo que carregamos no início
        delete rootNo; 
    }
}

/**
 * @brief (search) A mais fácil.
 *
 * Lógica: É só um 'while' que desce a árvore. Em cada nó,
 * ela acha o caminho certo ('i') pra descer.
 * Se em algum momento 'key == current->vetorChaves[i]', achou (true).
 * Se chegar na folha ('ehFolha') e não achar, não existe (false).
 */
long BPlusTreeInt::search(int key){

    if (idRaiz == -1) {
        return -1; // Árvore vazia
    }

    No* current = new No();
    lerNo(idRaiz, current);

    while (true) {
        
        // Encontra o primeiro índice 'i' onde key <= current->vetorChaves[i]
        int i = 0;
        while (i < current->numChaves && key > current->vetorChaves[i]) {
            i++;
        }
        // Agora, 'i' é o índice do primeiro elemento >= key

        if (current->ehFolha) {
            // --- Está na folha ---
            // Verifica se a chave no índice 'i' é a que procuramos
            if (i < current->numChaves && current->vetorChaves[i] == key) {
                // Achou! Retorna o ponteiro de dado.
                long dataPtr = current->vetorApontadores[i];
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
            if (i < current->numChaves && current->vetorChaves[i] == key) {
                 i++;
            }
            
            // 'i' é o índice do filho para onde descer.
            long childPosition = current->vetorApontadores[i];
            
            // Reutiliza o ponteiro 'current' para carregar o filho.
            lerNo(childPosition, current); 
        }
    }
}
