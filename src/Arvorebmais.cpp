// Bibliotecas padrão necessárias
#include <algorithm>          // Para funções como std::lower_bound (útil na busca)
#include <iostream>           // Para std::cout (impressão na tela)
#include <vector>             // Para std::vector (nossas listas dinâmicas)
#include <cstring>            // Para memcpy
#include <math.h>             // Para floor (na definição de 'm')
#include <cmath> 
#include "GerenciaBlocos.hpp" // Gerenciamento de blocos em disco
#include "Arvorebmais.hpp"    // Declaração da classe BPlusTreeInt

/*
 * ==========================================================
 * Implementação dos Métodos da Classe BPlusTreeInt
 * ==========================================================
 */

// Constructor
BPlusTreeInt::BPlusTreeInt(const std::string &nomeArquivo, const size_t tamanhoBloco_arg)
    : tamanhoBloco(static_cast<int>(tamanhoBloco_arg)),
      nomeArquivo(nomeArquivo),
      gerenciador(nomeArquivo, tamanhoBloco_arg),
      idRaiz(-1),
      totalBlocos(0)
{
    try
    {
        // se a arvore existe o arquivo tem algo
        if (gerenciador.getTamanhoArquivo() > 0)
        {
            lerCabecalho();

            // Verifica se o tamanho do bloco no arquivo bate com o fornecido
            if (static_cast<int>(tamanhoBloco_arg) != this->tamanhoBloco)
            {
                throw std::runtime_error("Erro: O tamanho do bloco fornecido é inconsistente com o do arquivo!");
            }
        }
        else
        {
            // Caso o arquivo nao exista, criar o cabecalho
            this->idRaiz = -1;
            this->totalBlocos = 1;       // total de blocos para o cabecalho / total de bloco para a classe

            // Escreve o cabecalho no arquivo.
            escreverCabecalho();
        }

        // Calcula 'm' depois de tudo estar definido.
        int tamMetadados = sizeof(bool) //eh_folha
                         + sizeof(int)  // numChaves
                         + sizeof(long) // proximo
                         + sizeof(long); //selfId
        m = floor((this->tamanhoBloco - tamMetadados) / (sizeof(int) + sizeof(long)));
    }
    catch (const std::exception &e)
    {
        // Re-lança a exceção para que o programa que chamou o construtor saiba que algo deu errado.
        throw;
    }
}

// Converte um objeto No para um buffer de bytes (CORRIGIDO)
void BPlusTreeInt::serializaNo(const No &No, char *buffer)
{
    char *ptr = buffer;

    // 1. Copia os campos de tamanho fixo (Isto estava correto)
    memcpy(ptr, &No.ehFolha, sizeof(bool));
    ptr += sizeof(bool);
    memcpy(ptr, &No.numChaves, sizeof(int));
    ptr += sizeof(int);
    memcpy(ptr, &No.proximo, sizeof(long));
    ptr += sizeof(long);
    memcpy(ptr, &No.selfId, sizeof(long));
    ptr += sizeof(long);

    // 2. Copia o conteúdo do vector de chaves (Isto estava correto)
    memcpy(ptr, No.vetorChaves.data(), No.numChaves * sizeof(int));
    ptr += No.numChaves * sizeof(int);

    // 3. Copia os ponteiros (de dados OU de filhos)
    if (No.ehFolha)
    {
        // Se é folha, salva 'numChaves' ponteiros de DADOS
        memcpy(ptr, No.vetorApontadores.data(), No.numChaves * sizeof(long));
        // (Opcional) Mover o ptr: ptr += No.numChaves * sizeof(long);
    }
    else
    {
        // Se é interno, salva 'numChaves + 1' ponteiros de FILHOS
        memcpy(ptr, No.vetorApontadores.data(), (No.numChaves + 1) * sizeof(long));
        // (Opcional) Mover o ptr: ptr += (No.numChaves + 1) * sizeof(long);
    }
}

// Converte um buffer de bytes de volta para um objeto No (CORRIGIDO)
void BPlusTreeInt::deserializaNo(const char *buffer, No &No)
{
    const char *ptr = buffer;

    // 1. Lê os campos de tamanho fixo (Isto estava correto)
    memcpy(&No.ehFolha, ptr, sizeof(bool));
    ptr += sizeof(bool);
    memcpy(&No.numChaves, ptr, sizeof(int));
    ptr += sizeof(int);
    memcpy(&No.proximo, ptr, sizeof(long));
    ptr += sizeof(long);
    memcpy(&No.selfId, ptr, sizeof(long)); // 
    ptr += sizeof(long);                   //

    // 2. Lê as chaves (Isto estava correto)
    No.vetorChaves.resize(No.numChaves);
    memcpy(No.vetorChaves.data(), ptr, No.numChaves * sizeof(int));
    ptr += No.numChaves * sizeof(int);

    // 3. Lê os ponteiros (de dados OU de filhos)
    if (No.ehFolha)
    {
        // Se é folha, lê 'numChaves' ponteiros de DADOS
        No.vetorApontadores.resize(No.numChaves);
        memcpy(No.vetorApontadores.data(), ptr, No.numChaves * sizeof(long));
    }
    else
    {
        // Se é interno, lê 'numChaves + 1' ponteiros de FILHOS
        No.vetorApontadores.resize(No.numChaves + 1);
        memcpy(No.vetorApontadores.data(), ptr, (No.numChaves + 1) * sizeof(long));
    }
}

// Escreve um nó no disco
void BPlusTreeInt::escreverNo(No *No)
{
    char *buffer = new char[tamanhoBloco];
    serializaNo(*No, buffer);
    gerenciador.escreveBloco(No->selfId, buffer);
    delete[] buffer;
}

// Lê um nó do disco e o desserializa
No *BPlusTreeInt::lerNo(long blockNum, No *No)
{
    char *buffer = new char[tamanhoBloco];
    gerenciador.lerBloco(blockNum, buffer);
    deserializaNo(buffer, *No);
    No->selfId = blockNum;
    delete[] buffer;
    return No;
}

//  Escreve o cabecalho da árvore B+ no bloco 0 do arquivo de índice.
void BPlusTreeInt::escreverCabecalho()
{
    // 1. Cria e preenche a struct com os dados atuais.
    cabecalho hdr;
    hdr.idRaiz = this->idRaiz;
    hdr.tamanhoBloco = this->tamanhoBloco;
    hdr.numBlocos = this->totalBlocos;

    // 2. Aloca um buffer na heap.
    char *buffer = new char[tamanhoBloco];

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
void BPlusTreeInt::lerCabecalho()
{
    // 1. Aloca um buffer na heap para receber os dados.
    char *buffer = new char[tamanhoBloco];

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
    this->totalBlocos = hdr.numBlocos;
}


// Retorna um novo ID para um nó baseado no total de blocos
long BPlusTreeInt::getNovoId()
{
    long id = this->totalBlocos; // o novo id é o total de blocos atual
    this->totalBlocos++; // incrementa o total de blocos
    return id; // retorna o id novo
}


/**
 * @brief (splitChild) REESCRITO PARA DISCO
 *
 * @param parent O nó pai (JÁ EM MEMÓRIA) que será modificado.
 * @param childIndex O índice (em parent->vetorApontadores) do filho que está lotado.
 **/
void BPlusTreeInt::splitChild(No *parent, int childIndex)
{

    // 1. Cria o novo "irmão" (newSibling) e aloca um bloco para ele no disco
    No *newSibling = new No();
    newSibling->selfId = this->getNovoId(); // atribui um novo id para o novo nó

    // 2. Carrega o filho lotado ('child') do disco
    long childOffset = parent->vetorApontadores[childIndex];
    No *child = new No();
    lerNo(childOffset, child);

    // 3. Define o status do novo irmão (folha ou não)
    newSibling->ehFolha = child->ehFolha;

    // 4. Calcula o "ponto do meio" para o split
    int middleIndex;
    int keyToPromote; // A chave que vai subir para o 'parent'

    if (child->ehFolha)
    {
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
    }
    else
    {
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
void BPlusTreeInt::insertNonFull(No *noAtual, int key, long dataPointer)
{
    // ==========================================================
    // CASO BASE (Recursão para aqui)
    // ==========================================================
    if (noAtual->ehFolha)
    {
        // 1. Encontra a posição correta para inserir a nova chave (key).
        // Usamos um loop (como solicitado) para encontrar o primeiro índice 'i'
        // onde a chave existente é MAIOR ou IGUAL à nova chave.
        // Isto imita o comportamento de std::upper_bound, garantindo que
        // chaves duplicadas sejam inseridas após as existentes.
        int i = 0;
        while (i < noAtual->numChaves && key >= noAtual->vetorChaves[i])
        {
            i++;
        }

        // 2. Insere a chave e o ponteiro de dado nos vetores.
        // O nó folha armazena pares (chave, ponteiro_de_dado).
        noAtual->vetorChaves.insert(noAtual->vetorChaves.begin() + i, key);
        noAtual->vetorApontadores.insert(noAtual->vetorApontadores.begin() + i, dataPointer);
        
        // 3. Atualiza a contagem de chaves.
        noAtual->numChaves++;

        // 4. Salva o nó folha modificado de volta no disco.
        // (Este nó não estava cheio, então não precisa de split).
        escreverNo(noAtual);
        return; // A inserção termina aqui.
    }
    // ==========================================================
    // CASO RECURSIVO (Nó interno)
    // ==========================================================
    else
    {
        // Nó é interno. Encontra o filho correto para descer.

        // 1. Procura o índice do filho correto ('i').
        // O loop encontra o primeiro índice 'i' onde a chave a ser inserida (key)
        // é MENOR que a chave no nó (vetorChaves[i]).
        int i = 0;
        while (i < noAtual->numChaves && key >= noAtual->vetorChaves[i])
        {
            i++;
        }
        
        // 'i' agora é o índice do ponteiro em 'vetorApontadores'
        // para o qual devemos descer.

        // ==========================================================
        // A "MÁGICA" DA B+ TREE: Dividir antes de descer
        // ==========================================================

        // 2. Carrega o filho 'i' do disco para checar se está lotado
        long childOffset = noAtual->vetorApontadores[i];
        No *childNo = new No();
        lerNo(childOffset, childNo);

        // 3. Define a capacidade máxima do filho
        bool childIsFull = false;
        if (childNo->ehFolha)
        {
            if (childNo->numChaves == m)
            { // Folha lota com 'm'
                childIsFull = true;
            }
        }
        else
        {
            if (childNo->numChaves == m - 1)
            { // Interno lota com 'm-1'
                childIsFull = true;
            }
        }

        // 4. Se o filho estiver lotado, divide ele AGORA.
        if (childIsFull)
        {
            // 'noAtual' é o pai (em memória), 'i' é o índice do filho
            splitChild(noAtual, i);

            // O split ALTEROU o 'noAtual' (pai), adicionando uma chave
            // e um novo filho.
            // Precisamos checar se a 'key' agora pertence
            // ao novo irmão (que está em 'i+1').
            if (key > noAtual->vetorChaves[i])
            {
                i++;
            }

            // O 'childNo' que tínhamos carregado está obsoleto/dividido.
            // Recarregamos o nó filho correto (o original [i] ou o novo [i+1])
            // que agora com certeza tem espaço.
            childOffset = noAtual->vetorApontadores[i];
            lerNo(childOffset, childNo);
        }

        // 5. Agora temos CERTEZA que o 'childNo' (filho[i]) tem espaço.
        // Chamamos a recursão para descer um nível.
        insertNonFull(childNo, key, dataPointer);

        // 6. Limpa a memória (o 'childNo' que foi carregado neste nível)
        delete childNo;
    }
}

/**
 * @brief (insert - public) A que o usuário chama.
 * TRADUZIDO PARA DISCO
 **/
void BPlusTreeInt::insert(int key, long dataPointer){

    // CASO 1: Árvore vazia. Eu só crio a 'root' como folha
    // e coloco a chave.
    if (idRaiz == -1){
        No *rootNo = new No(true); // Cria a raiz, que também é uma folha.
        rootNo->selfId = this->getNovoId(); // atribui o primeiro id para a raiz
        this->idRaiz = rootNo->selfId; // id dele vai ser a posição onde ele tá no arq

        //adiciono a chave e o apontador a ela
        rootNo->vetorChaves.push_back(key);
        rootNo->vetorApontadores.push_back(dataPointer);
        rootNo->numChaves = 1;

        escreverNo(rootNo);
        escreverCabecalho();
        delete rootNo;
        return;
    }
    // CASO 2: Árvore já existe.
    else{
        // 1. Carrega o nó raiz ATUAL do disco para a memória.
        No *rootNo = new No();
        lerNo(this->idRaiz, rootNo);

        // 2. Verifica se a raiz está lotada
        bool rootIsFull = false;
        if (rootNo->ehFolha){
            if (rootNo->numChaves == m)
            { // Folha lota com 'm'
                rootIsFull = true;
            }
        }
        else
        {
            if (rootNo->numChaves == m - 1)
            { // Interno lota com 'm-1'
                rootIsFull = true;
            }
        }

        // Sub-caso: A *raiz* tá lotada.
        // A raiz não tem pai, então eu não posso só chamar 'splitChild'.
        if (rootIsFull){
            // A. Cria uma NOVA raiz (na memória)
            No *newRootNo = new No(false); // Nova raiz não eh folha (porque vai ter filhos)
            newRootNo->selfId = this->getNovoId();

            // B. O primeiro filho da nova raiz é a RAIZ ANTIGA (a posição dela)
            newRootNo->vetorApontadores.push_back(this->idRaiz);
            // newRootNo agora tem 1 apontador, mas numChaves permanece 0.

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
        else
        {
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
long BPlusTreeInt::search(int key)
{

    if (idRaiz == -1)
    {
        return -1; // Árvore vazia
    }

    No *current = new No();
    lerNo(idRaiz, current);

    while (true)
    {

        // Encontra o primeiro índice 'i' onde key <= current->vetorChaves[i]
        int i = 0;
        while (i < current->numChaves && key > current->vetorChaves[i])
        {
            i++;
        }
        // Agora, 'i' é o índice do primeiro elemento >= key

        if (current->ehFolha)
        {
            // --- Está na folha ---
            // Verifica se a chave no índice 'i' é a que procuramos
            if (i < current->numChaves && current->vetorChaves[i] == key)
            {
                // Achou! Retorna o ponteiro de dado.
                long dataPtr = current->vetorApontadores[i];
                delete current;
                return dataPtr;
            }
            else
            {
                // Não achou.
                delete current;
                return -1;
            }
        }
        else
        {
            // --- É nó interno ---

            // Se a chave for EXATAMENTE igual a uma chave interna, na B+ Tree
            // o valor está na sub-árvore da DIREITA daquela chave.
            if (i < current->numChaves && current->vetorChaves[i] == key)
            {
                i++;
            }

            // 'i' é o índice do filho para onde descer.
            long childPosition = current->vetorApontadores[i];

            // Reutiliza o ponteiro 'current' para carregar o filho.
            lerNo(childPosition, current);
        }
    }
}

// ==========================================================
// NOVAS FUNÇÕES PARA REMOÇÃO
// ==========================================================

// Função pública que inicia a remoção
void BPlusTreeInt::remove(int key) {
    if (idRaiz == -1) {
        std::cout << "Árvore vazia. Não é possível remover." << std::endl;
        return;
    }

    // Carrega a raiz
    No* rootNo = new No();
    lerNo(idRaiz, rootNo);

    // Chama a função recursiva de remoção
    removeRecursivo(rootNo, key);

    // --- Tratamento especial para a raiz ---
    // Se a raiz ficou vazia após a remoção (acontece se a raiz antiga foi fundida com seu único filho)
    if (rootNo->numChaves == 0) {
        // Se a raiz era uma folha, a árvore está vazia agora
        if (rootNo->ehFolha) {
            idRaiz = -1; // Marca a árvore como vazia
            // Poderíamos deletar o bloco 1 do arquivo aqui, mas não é estritamente necessário
            // e complica a alocação de novos IDs. Deixaremos o bloco órfão por simplicidade.
        } else {
            // Se a raiz era interna e ficou vazia, a nova raiz é o seu (único) filho
            idRaiz = rootNo->vetorApontadores[0];
            // O bloco da raiz antiga (rootNo->selfId) fica órfão.
        }
        // Atualiza o cabeçalho no disco
        escreverCabecalho(); 
    }
    // --------------------------------------

    delete rootNo; // Libera a memória do nó raiz carregado
}

// Encontra o índice da chave 'key' em 'no', ou o índice onde ela deveria ser inserida
// (Usado tanto na busca quanto na remoção)
int BPlusTreeInt::findKeyIndex(No* no, int key) {
    int index = 0;
    // Procura o primeiro índice onde key <= no->vetorChaves[index]
    while (index < no->numChaves && no->vetorChaves[index] < key) {
        ++index;
    }
    return index;
}

// Função recursiva principal para remoção
void BPlusTreeInt::removeRecursivo(No* noAtual, int key) {
    int keyIndex = findKeyIndex(noAtual, key);

    // --- CASO 1: A chave está no nó ATUAL ---
    if (keyIndex < noAtual->numChaves && noAtual->vetorChaves[keyIndex] == key) {
        
        if (noAtual->ehFolha) {
            // --- Subcaso 1.1: Nó é folha e a chave está aqui ---
            removeFromLeaf(noAtual, keyIndex);
        } else {
            // --- Subcaso 1.2: Nó é interno e a chave está aqui ---
            removeFromInternal(noAtual, keyIndex);
        }
    } 
    // --- CASO 2: A chave NÃO está no nó atual ---
    else {
        // Se for folha, a chave não existe na árvore
        if (noAtual->ehFolha) {
            std::cout << "Chave " << key << " não encontrada na árvore." << std::endl;
            return;
        }

        // --- Subcaso 2.1: Nó é interno, desce para o filho apropriado ---
        
        // 'keyIndex' agora indica para qual filho descer
        
        // Verifica se é o último ponteiro (caso a chave seja maior que todas no nó)
        bool ehUltimoFilho = (keyIndex == noAtual->numChaves);

        // --- Passo Crucial: Garantir que o filho terá chaves suficientes *antes* de descer ---
        // Carrega o filho para verificar seu tamanho
        long childId = noAtual->vetorApontadores[keyIndex];
        No* childNo = new No();
        lerNo(childId, childNo);

        // Calcula o número mínimo de chaves permitido
        int minKeys;
        if (childNo->ehFolha) {
            // Para folhas, o mínimo é ceil(m / 2.0) - 1 ??? OU floor((m-1)/2) ??? -> Ver Cormen/Definição B+
            // Assumindo mínimo de chaves = floor(m/2) para folhas B+
             minKeys = m / 2; // Arredonda para baixo
        } else {
            // Para internos, o mínimo é ceil((m-1) / 2.0) ??? OU ceil(m/2)-1 ??? -> Ver Cormen/Definição B+
            // Assumindo mínimo de chaves = ceil(m/2) - 1 para internos B+ (que têm m filhos)
            minKeys = (m + 1) / 2 -1; // ceil(m/2) - 1
        }


        // Se o filho está no limite mínimo, precisa de tratamento (fillNode)
        if (childNo->numChaves == minKeys) {

            

            fillNode(noAtual, keyIndex); // Tenta emprestar ou fazer merge

            // fillNode pode ter modificado o pai (noAtual) e a estrutura abaixo dele.
            // Precisamos re-calcular o índice do filho para onde descer,
            // pois a chave pode ter se movido para um nó diferente após merge/borrow.
            // OBS: Esta é uma simplificação. Uma implementação robusta revalidaria o caminho.
            // Por simplicidade aqui, vamos assumir que após fillNode, descemos pelo mesmo índice
            // ou pelo índice anterior se houve merge com o esquerdo.
            // A chamada recursiva *dentro* de fillNode (em merge) já deve ter tratado a chave.
            // Se fillNode fez borrow, a recursão continua normalmente.
            // Se fillNode fez merge, a recursão já aconteceu no nó fundido.
            // Então, se fillNode foi chamado, a remoção já foi tratada abaixo.
             delete childNo;
             return; // A recursão já foi feita dentro de fillNode/merge se necessário

        } else {
             // O filho tem chaves suficientes, desce recursivamente
             removeRecursivo(childNo, key);
        }
       
        delete childNo; // Libera a memória do filho carregado neste nível
    }
}


// Remove a chave do nó folha
void BPlusTreeInt::removeFromLeaf(No* folha, int keyIndex) {
    // Simplesmente remove a chave e o ponteiro de dados correspondente
    folha->vetorChaves.erase(folha->vetorChaves.begin() + keyIndex);
    folha->vetorApontadores.erase(folha->vetorApontadores.begin() + keyIndex);
    folha->numChaves--;

    // Salva a folha modificada no disco
    escreverNo(folha);
}

// Remove a chave do nó interno
void BPlusTreeInt::removeFromInternal(No* interno, int keyIndex) {
    int key = interno->vetorChaves[keyIndex];

    // Carrega os filhos esquerdo e direito da chave a ser removida
    long leftChildId = interno->vetorApontadores[keyIndex];
    long rightChildId = interno->vetorApontadores[keyIndex + 1];
    No* leftChild = new No();
    No* rightChild = new No();
    lerNo(leftChildId, leftChild);
    lerNo(rightChildId, rightChild);

    // Calcula o número mínimo de chaves permitido (igual a removeRecursivo)
    int minKeys;
     if (leftChild->ehFolha) { // Se um é folha, o outro também é (na B+)
          minKeys = m / 2; 
     } else {
          minKeys = (m + 1) / 2 -1; 
     }

    // --- CASO 2a: O filho esquerdo tem chaves suficientes (> minKeys) ---
    if (leftChild->numChaves > minKeys) {
        // Encontra o predecessor (a maior chave na subárvore esquerda)
        int pred = getPred(interno, keyIndex);
        // Substitui a chave 'key' no nó interno pelo seu predecessor
        interno->vetorChaves[keyIndex] = pred;
        escreverNo(interno); // Salva o nó interno modificado
        // Recursivamente remove o predecessor de onde ele veio (na subárvore esquerda)
        removeRecursivo(leftChild, pred); 
    }
    // --- CASO 2b: O filho direito tem chaves suficientes (> minKeys) ---
    else if (rightChild->numChaves > minKeys) {
        // Encontra o sucessor (a menor chave na subárvore direita)
        int succ = getSucc(interno, keyIndex);
        // Substitui a chave 'key' no nó interno pelo seu sucessor
        interno->vetorChaves[keyIndex] = succ;
        escreverNo(interno); // Salva o nó interno modificado
        // Recursivamente remove o sucessor de onde ele veio (na subárvore direita)
        removeRecursivo(rightChild, succ);
    }
    // --- CASO 2c: Ambos os filhos estão no limite mínimo (minKeys) ---
    else {
        // Faz o merge do filho esquerdo, a chave 'key' do pai, e o filho direito
        // A função merge cuidará de remover 'key' do pai e chamar a recursão
        merge(interno, keyIndex); 
    }

    delete leftChild;
    delete rightChild;
}

// Encontra a maior chave na subárvore enraizada no filho 'index' do nó 'no'
int BPlusTreeInt::getPred(No* no, int index) {
    // Começa descendo para o filho esquerdo da chave 'index'
    long currentId = no->vetorApontadores[index];
    No* current = new No();
    lerNo(currentId, current);

    // Desce pela direita até encontrar uma folha
    while (!current->ehFolha) {
        long nextId = current->vetorApontadores[current->numChaves]; // Último ponteiro
        lerNo(nextId, current);
    }

    // A última chave da folha encontrada é o predecessor
    int pred = current->vetorChaves[current->numChaves - 1];
    delete current;
    return pred;
}

// Encontra a menor chave na subárvore enraizada no filho 'index + 1' do nó 'no'
int BPlusTreeInt::getSucc(No* no, int index) {
    // Começa descendo para o filho direito da chave 'index'
    long currentId = no->vetorApontadores[index + 1];
    No* current = new No();
    lerNo(currentId, current);

    // Desce pela esquerda até encontrar uma folha
    while (!current->ehFolha) {
        long nextId = current->vetorApontadores[0]; // Primeiro ponteiro
        lerNo(nextId, current);
    }

    // A primeira chave da folha encontrada é o sucessor
    int succ = current->vetorChaves[0];
    delete current;
    return succ;
}


// --- Funções para Lidar com Underflow ---

// Função chamada ANTES de descer para um filho que tem o número mínimo de chaves
void BPlusTreeInt::fillNode(No* pai, int indexFilho) {
    // Tenta emprestar do irmão esquerdo primeiro
    if (indexFilho != 0) { // Se não for o primeiro filho
        long prevSiblingId = pai->vetorApontadores[indexFilho - 1];
        No* prevSibling = new No();
        lerNo(prevSiblingId, prevSibling);

        // Calcula minKeys (igual a removeRecursivo)
         int minKeys;
         if (prevSibling->ehFolha) { 
              minKeys = m / 2; 
         } else {
              minKeys = (m + 1) / 2 -1; 
         }

        if (prevSibling->numChaves > minKeys) {
            borrowFromPrev(pai, indexFilho); // Empresta do esquerdo
            delete prevSibling;
            return; // Resolvido
        }
        delete prevSibling; // Não pode emprestar, libera memória
    }

    // Tenta emprestar do irmão direito
    if (indexFilho != pai->numChaves) { // Se não for o último filho
        long nextSiblingId = pai->vetorApontadores[indexFilho + 1];
        No* nextSibling = new No();
        lerNo(nextSiblingId, nextSibling);

        // Calcula minKeys (igual a removeRecursivo)
         int minKeys;
         if (nextSibling->ehFolha) { 
              minKeys = m / 2; 
         } else {
              minKeys = (m + 1) / 2 -1; 
         }


        if (nextSibling->numChaves > minKeys) {
            borrowFromNext(pai, indexFilho); // Empresta do direito
            delete nextSibling;
            return; // Resolvido
        }
        delete nextSibling; // Não pode emprestar, libera memória
    }

    // Se não pode emprestar de nenhum dos lados, faz merge
    if (indexFilho != pai->numChaves) {
        merge(pai, indexFilho); // Faz merge com o irmão direito
    } else {
        merge(pai, indexFilho - 1); // Faz merge com o irmão esquerdo (último filho)
    }
}

// Empresta uma chave do irmão esquerdo ('indexFilho - 1') para o filho ('indexFilho')
void BPlusTreeInt::borrowFromPrev(No* pai, int indexFilho) {
    long filhoId = pai->vetorApontadores[indexFilho];
    long prevSiblingId = pai->vetorApontadores[indexFilho - 1];
    No* filho = new No();
    No* prevSibling = new No();
    lerNo(filhoId, filho);
    lerNo(prevSiblingId, prevSibling);

    // --- Caso: Nós são internos ---
    if (!filho->ehFolha) {
        // 1. Move a chave separadora do pai para o início do filho
        filho->vetorChaves.insert(filho->vetorChaves.begin(), pai->vetorChaves[indexFilho - 1]);
        
        // 2. Move o último ponteiro do irmão esquerdo para o início do filho
        filho->vetorApontadores.insert(filho->vetorApontadores.begin(), prevSibling->vetorApontadores.back());
        prevSibling->vetorApontadores.pop_back();

        // 3. Move a última chave do irmão esquerdo para o pai (substituindo a que desceu)
        pai->vetorChaves[indexFilho - 1] = prevSibling->vetorChaves.back();
        prevSibling->vetorChaves.pop_back();

    } 
    // --- Caso: Nós são folhas ---
    else {
        // 1. Copia a última chave do irmão esquerdo para o início do filho
        filho->vetorChaves.insert(filho->vetorChaves.begin(), prevSibling->vetorChaves.back());
        // 2. Copia o último ponteiro de dados do irmão esquerdo para o início do filho
        filho->vetorApontadores.insert(filho->vetorApontadores.begin(), prevSibling->vetorApontadores.back());
        prevSibling->vetorChaves.pop_back();
        prevSibling->vetorApontadores.pop_back();

        // 3. Atualiza a chave no pai para ser a *nova* primeira chave do filho (que veio do irmão)
        pai->vetorChaves[indexFilho - 1] = filho->vetorChaves[0];
    }

    // Atualiza contadores
    filho->numChaves++;
    prevSibling->numChaves--;

    // Salva as alterações no disco
    escreverNo(pai);
    escreverNo(filho);
    escreverNo(prevSibling);

    delete filho;
    delete prevSibling;
}

void BPlusTreeInt::borrowFromNext(No* pai, int indexFilho) {
    long filhoId = pai->vetorApontadores[indexFilho];
    long nextSiblingId = pai->vetorApontadores[indexFilho + 1];
    No* filho = new No();
    No* nextSibling = new No();
    lerNo(filhoId, filho);
    lerNo(nextSiblingId, nextSibling);

    if (!filho->ehFolha) {
        filho->vetorChaves.push_back(pai->vetorChaves[indexFilho]);

        filho->vetorApontadores.push_back(nextSibling->vetorApontadores.front());
        nextSibling->vetorApontadores.erase(nextSibling->vetorApontadores.begin());

        pai->vetorChaves[indexFilho] = nextSibling->vetorChaves.front();
        nextSibling->vetorChaves.erase(nextSibling->vetorChaves.begin());
    }
    else {
        filho->vetorChaves.push_back(nextSibling->vetorChaves.front());
        filho->vetorApontadores.push_back(nextSibling->vetorApontadores.front());
        nextSibling->vetorChaves.erase(nextSibling->vetorChaves.begin());
        nextSibling->vetorApontadores.erase(nextSibling->vetorApontadores.begin());

        pai->vetorChaves[indexFilho] = nextSibling->vetorChaves[0];
    }

    filho->numChaves++;
    nextSibling->numChaves--;

    escreverNo(pai);
    escreverNo(filho);
    escreverNo(nextSibling);

    delete filho;
    delete nextSibling;
}

void BPlusTreeInt::merge(No* pai, int indexFilho) {
    long leftChildId = pai->vetorApontadores[indexFilho];
    long rightChildId = pai->vetorApontadores[indexFilho + 1];
    No* leftChild = new No();
    No* rightChild = new No();
    lerNo(leftChildId, leftChild);
    lerNo(rightChildId, rightChild);

    if (!leftChild->ehFolha) {
        
        leftChild->vetorChaves.push_back(pai->vetorChaves[indexFilho]);

        leftChild->vetorChaves.insert(leftChild->vetorChaves.end(), 
                                      rightChild->vetorChaves.begin(), 
                                      rightChild->vetorChaves.end());

        leftChild->vetorApontadores.insert(leftChild->vetorApontadores.end(), 
                                          rightChild->vetorApontadores.begin(), 
                                          rightChild->vetorApontadores.end());
    }

    else {

        leftChild->vetorChaves.insert(leftChild->vetorChaves.end(), 
                                      rightChild->vetorChaves.begin(), 
                                      rightChild->vetorChaves.end());
                                      
        leftChild->vetorApontadores.insert(leftChild->vetorApontadores.end(), 
                                          rightChild->vetorApontadores.begin(), 
                                          rightChild->vetorApontadores.end());
        
        leftChild->proximo = rightChild->proximo;
    }

    leftChild->numChaves += rightChild->numChaves + (leftChild->ehFolha ? 0 : 1);

    pai->vetorChaves.erase(pai->vetorChaves.begin() + indexFilho);
    pai->vetorApontadores.erase(pai->vetorApontadores.begin() + indexFilho + 1);
    pai->numChaves--;

    escreverNo(pai);
    escreverNo(leftChild);

    delete leftChild;
    delete rightChild;
}