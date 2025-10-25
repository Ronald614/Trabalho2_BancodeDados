// Bibliotecas padrão necessárias
#include <vector>             // Para std::vector usado em No
#include <cstring>            // Para memcpy e memset
#include <stdexcept>          // Para exceções padrão usados no construtor
#include <math.h>             // Para floor (na definição de 'm')
#include "GerenciaBlocos.hpp" // Gerenciamento de blocos em disco
#include "Arvorebmais.hpp"    // Declaração da classe BPlusTreeInt

/*
 * ==========================================================
 * Implementação dos Métodos da Classe BPlusTreeInt
 * ==========================================================
 */

// Construtor da árvore B+
BPlusTreeInt::BPlusTreeInt(const std::string &nomeArquivo, const size_t tamanhoBloco_arg)
    : tamanhoBloco(static_cast<int>(tamanhoBloco_arg)), 
      nomeArquivo(nomeArquivo),
      gerenciador(nomeArquivo, tamanhoBloco_arg),
      idRaiz(-1),
      totalBlocos(0)
{
    try
    {
        // se a arvore existe o arquivo tem algo no cabecalho
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
            this->idRaiz = -1;            // arvore vazia
            this->totalBlocos = 1;       // total de blocos para o cabecalho / total de bloco para a classe

            // Escreve o cabecalho no arquivo
            escreverCabecalho();
        }

        // Calcula 'm' depois de tudo estar definido.
        int tamMetadados = sizeof(bool) //eh_folha
                         + sizeof(int)  // numChaves
                         + sizeof(long); // proximo
        m = floor((this->tamanhoBloco - tamMetadados) / (sizeof(int) + sizeof(long)));
    }
    catch (const std::exception &e)
    {
        // Re-lança a exceção para que o programa que chamou o construtor saiba que algo deu errado
        throw;
    }
}

// Converte um objeto No para um buffer de bytes
void BPlusTreeInt::serializaNo(const No &No, char *buffer)
{
    char *ptr = buffer;

    // Copia os campos de tamanho fixo
    memcpy(ptr, &No.ehFolha, sizeof(bool));
    ptr += sizeof(bool);
    memcpy(ptr, &No.numChaves, sizeof(int));
    ptr += sizeof(int);
    memcpy(ptr, &No.proximo, sizeof(long));
    ptr += sizeof(long);

    //Copia o conteúdo do vector de chaves
    // .data() retorna um ponteiro para o array interno do vector
    memcpy(ptr, No.vetorChaves.data(), No.numChaves * sizeof(int));
    ptr += No.numChaves * sizeof(int);

    // Copia o conteúdo do vector de apontadores
    if (No.ehFolha)
    {
        // Se é folha, contem M chaves e M apontadores de DADOS
        memcpy(ptr, No.vetorApontadores.data(), No.numChaves * sizeof(long));
        // (Opcional) Mover o ptr: ptr += No.numChaves * sizeof(long);
    }
    else
    {
        // Se é interno, contém M-1 chaves e M apontadores de FILHOS
        memcpy(ptr, No.vetorApontadores.data(), (No.numChaves + 1) * sizeof(long));
        // (Opcional) Mover o ptr: ptr += (No.numChaves + 1) * sizeof(long);
    }
}

// Converte um buffer de bytes de volta para um objeto No
void BPlusTreeInt::deserializaNo(const char *buffer, No &No)
{
    const char *ptr = buffer;

    //Lê os campos de tamanho fixo
    memcpy(&No.ehFolha, ptr, sizeof(bool));
    ptr += sizeof(bool);
    memcpy(&No.numChaves, ptr, sizeof(int));
    ptr += sizeof(int);
    memcpy(&No.proximo, ptr, sizeof(long));
    ptr += sizeof(long);

    // Lê as chave 
    No.vetorChaves.resize(No.numChaves); // ajusta o tamanho do vetor
    memcpy(No.vetorChaves.data(), ptr, No.numChaves * sizeof(int)); // copia as chaves
    ptr += No.numChaves * sizeof(int);

    // Lê os apontadores
    if (No.ehFolha)
    {
        // Se é folha, contém 'numChaves' ponteiros de DADOS
        No.vetorApontadores.resize(No.numChaves);
        memcpy(No.vetorApontadores.data(), ptr, No.numChaves * sizeof(long));
    }
    else
    {
        // Se é interno, contem 'numChaves + 1' ponteiros de FILHOS
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
    // Adiciona os metadados ao buffer e escreve no bloco 0.
    cabecalho hdr;
    hdr.idRaiz = this->idRaiz;
    hdr.tamanhoBloco = this->tamanhoBloco;
    hdr.numBlocos = this->totalBlocos;

    // Aloca um buffer na heap para o bloco
    char *buffer = new char[tamanhoBloco];

    // Importante zera o buffer para não ter lixo além do cabecalho
    memset(buffer, 0, tamanhoBloco);

    // Copia a struct cabecalho para o buffer
    memcpy(buffer, &hdr, sizeof(cabecalho));

    // Escreve o buffer no bloco 0 do arquivo
    gerenciador.escreveBloco(0, buffer);

    //Libera a memória alocada.
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
    // o total de blocos base é 1 (bloco do cabecalho) gravado no construtor
    // ou o lido do arquivo se ja existia
    this->totalBlocos++; // incrementa o total de blocos
    return id; // retorna o id novo
}

/**
 * @brief (splitChild) REESCRITO PARA DISCO
 *
 * @param parent O nó pai (JÁ EM MEMÓRIA) que será modificado.
 * @param irmaoIndex O índice (em parent->vetorApontadores) do filho que está lotado.
 **/
void BPlusTreeInt::splitChild(No *parent, int irmaoIndex)
{

    // 1. Cria o novo "irmão" (novoIrmao) e aloca um bloco para ele no disco
    No *novoIrmao = new No();
    novoIrmao->selfId = this->getNovoId(); // atribui um novo id para o novo nó

    // 2. Carrega o filho lotado ('irmao') do disco
    long idFilho = parent->vetorApontadores[irmaoIndex];
    No *irmao = new No();
    lerNo(idFilho, irmao);

    // 3. Define o status do novo irmão (folha ou não)
    novoIrmao->ehFolha = irmao->ehFolha;

    // 4. Calcula o "ponto do meio" para o split
    int indiceMeio;
    int chavePromovida; // A chave que vai subir para o 'parent'

    if (irmao->ehFolha)
    {
        // --- Split de NÓ FOLHA ---
        // 'm' é o n° max de pares. Um nó folha lotado tem 'm' chaves.
        // Ponto do meio: m / 2.
        indiceMeio = m / 2;

        // A chave no 'indiceMeio' é a primeira a ir para o novo irmão.
        // Em uma B+ Tree, a chave promovida de uma folha é COPIADA, não movida.
        chavePromovida = irmao->vetorChaves[indiceMeio];

        // Copia a "metade direita" (chaves) para o novo irmão
        novoIrmao->vetorChaves.assign(irmao->vetorChaves.begin() + indiceMeio, irmao->vetorChaves.end());
        // Copia a "metade direita" (ponteiros de DADOS)
        novoIrmao->vetorApontadores.assign(irmao->vetorApontadores.begin() + indiceMeio, irmao->vetorApontadores.end());

        // Atualiza os contadores de chaves
        novoIrmao->numChaves = m - indiceMeio;
        irmao->numChaves = indiceMeio;

        // Atualiza os vetores do filho antigo (trunca)
        irmao->vetorChaves.resize(indiceMeio);
        irmao->vetorApontadores.resize(indiceMeio);

        // Atualiza a lista ligada de folhas
        novoIrmao->proximo = irmao->proximo;
        irmao->proximo = novoIrmao->selfId;
    }
    else
    {
        // --- Split de NÓ INTERNO ---
        // Um nó interno lotado tem 'm-1' chaves (e 'm' filhos).
        // Ponto do meio: (m-1) / 2
        indiceMeio = (m - 1) / 2;

        // A chave do meio é MOVIDA para o pai.
        chavePromovida = irmao->vetorChaves[indiceMeio];

        // Copia a "metade direita" (chaves) para o novo irmão
        // (sem incluir a chave promovida)
        novoIrmao->vetorChaves.assign(irmao->vetorChaves.begin() + indiceMeio + 1, irmao->vetorChaves.end());
        // Copia a "metade direita" (ponteiros de FILHOS)
        novoIrmao->vetorApontadores.assign(irmao->vetorApontadores.begin() + indiceMeio + 1, irmao->vetorApontadores.end());

        // Atualiza os contadores de chaves
        novoIrmao->numChaves = (m - 1) - (indiceMeio + 1);
        irmao->numChaves = indiceMeio;

        // Atualiza os vetores do filho antigo (trunca)
        irmao->vetorChaves.resize(indiceMeio);
        irmao->vetorApontadores.resize(indiceMeio + 1); // Nós internos têm numChaves+1 filhos
    }

    // 5. ATUALIZA O NÓ PAI (que já estava em memória)
    // Insere a chave promovida
    parent->vetorChaves.insert(parent->vetorChaves.begin() + irmaoIndex, chavePromovida);
    // Insere o ponteiro para o novo irmão
    parent->vetorApontadores.insert(parent->vetorApontadores.begin() + irmaoIndex + 1, novoIrmao->selfId);
    parent->numChaves++;

    // 6. ESCREVE OS 3 NÓS MODIFICADOS DE VOLTA NO DISCO
    escreverNo(parent);
    escreverNo(irmao);
    escreverNo(novoIrmao);

    // 7. Limpa a memória
    delete irmao;
    delete novoIrmao;
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
    // CASO BASE, está em uma folha
    if (noAtual->ehFolha)
    {
        //Anda ate onde chave >= vetorChaves[i]
        //admite chaves duplicadas
        int i = 0;
        while (i < noAtual->numChaves && key >= noAtual->vetorChaves[i])
        {
            i++;
        }

        // Adiciona a chave e o apontador na posicao 'i'
        noAtual->vetorChaves.insert(noAtual->vetorChaves.begin() + i, key);
        noAtual->vetorApontadores.insert(noAtual->vetorApontadores.begin() + i, dataPointer);
        
        // Aumenta o contador de chaves do no inserido
        noAtual->numChaves++;

        // Salva o no olha no arquivo
        // para persistir a insercao
        escreverNo(noAtual);
        return; // A inserção termina aqui.
    }
   
    // Caso recursivo, não está em uma folha
    // preciso descer para o filho correto
    else
    {
        // Nó é interno. Encontra o filho correto para descer.

        // Procura o índice do filho correto ('i').
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
        long idFilho = noAtual->vetorApontadores[i];
        No *irmaoNo = new No();
        lerNo(idFilho, irmaoNo);

        // 3. Define a capacidade máxima do filho
        bool irmaoIsFull = false;
        if (irmaoNo->ehFolha)
        {
            if (irmaoNo->numChaves == m)
            { // Folha lota com 'm'
                irmaoIsFull = true;
            }
        }
        else
        {
            if (irmaoNo->numChaves == m - 1)
            { // Interno lota com 'm-1'
                irmaoIsFull = true;
            }
        }

        // 4. Se o filho estiver lotado, divide ele AGORA.
        if (irmaoIsFull)
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

            // O 'irmaoNo' que tínhamos carregado está obsoleto/dividido.
            // Recarregamos o nó filho correto (o original [i] ou o novo [i+1])
            // que agora com certeza tem espaço.
            idFilho = noAtual->vetorApontadores[i];
            lerNo(idFilho, irmaoNo);
        }

        // 5. Agora temos CERTEZA que o 'irmaoNo' (filho[i]) tem espaço.
        // Chamamos a recursão para descer um nível.
        insertNonFull(irmaoNo, key, dataPointer);

        // 6. Limpa a memória (o 'irmaoNo' que foi carregado neste nível)
        delete irmaoNo;
    }
}

/**
 * @brief (insert - public) A que o usuário chama.
 * TRADUZIDO PARA DISCO
 **/
void BPlusTreeInt::insert(int key, long dataPointer){

    // CASO 1: Árvore vazia. Eu só crio a 'raiz' como folha
    // e coloco a chave e o apontador nela.
    if (idRaiz == -1){
        No *primeiraRaiz = new No(true); // Cria a raiz, que também é uma folha.
        primeiraRaiz->selfId = this->getNovoId(); // atribui o primeiro id para a raiz
        this->idRaiz = primeiraRaiz->selfId; // id dele vai ser a posição onde ele tá no arq

        //adiciono a chave e o apontador a ela
        primeiraRaiz->vetorChaves.push_back(key);
        primeiraRaiz->vetorApontadores.push_back(dataPointer);
        primeiraRaiz->numChaves = 1;

        escreverNo(primeiraRaiz);  // escreve a raiz no arquivo
        escreverCabecalho(); // porque a raiz mudou
        delete primeiraRaiz; 
        return;
    }
    // CASO 2: Árvore já existe.
    else{
        // Carrega o nó raiz ATUAL do disco para a memória.
        No *raizInicial = new No();
        lerNo(this->idRaiz, raizInicial);

        // Verifica se a raiz está lotada
        bool raizCheia = false;
        if (raizInicial->ehFolha){
            if (raizInicial->numChaves == m)
            { // Folha lota com 'm'
                raizCheia = true;
            }
        }
        else
        {
            if (raizInicial->numChaves == m - 1)
            { // Interno lota com 'm-1'
                raizCheia = true;
            }
        }

        // Sub-caso: A *raiz* tá lotada.
        // A raiz não tem pai, então eu não posso só chamar 'splitChild'.
        if (raizCheia){
            // Cria um novo nó para ser a nova raiz na memória
            No *novaRaiz = new No(false); // Nova raiz não eh folha (porque vai ter filhos)
            novaRaiz->selfId = this->getNovoId();

            // O primeiro filho e a antiga raiz (que tá lotada)
            novaRaiz->vetorApontadores.push_back(this->idRaiz);
            // newRootNo agora tem 1 apontador, mas numChaves permanece 0.

            //atualiza o id da raiz na arvore
            this->idRaiz = novaRaiz->selfId;
            escreverCabecalho();

            // chama o splitChild para dividir a antiga raiz
            // a nova raiz e o filho lotado no indice 0 (antiga raiz) 
            splitChild(novaRaiz, 0);

            // Agora a nova raiz tem 1 chave e 2 filhos.
            // A antiga raiz foi dividida em 2 nós com espaço 

            // Precisamos decidir para qual filho descer para inserir a nova chave.
            insertNonFull(novaRaiz, key, dataPointer);

            // Limpa a memória
            delete novaRaiz;
        }
        // Sub-caso: A raiz tem espaço.
        else
        {
            // A raiz tem espaço. só chama a inserção nela.
            insertNonFull(raizInicial, key, dataPointer);
        }
        // Limpa a raiz Inicial que carregamos no início
        delete raizInicial;
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

    No *noAtual = new No();
    lerNo(idRaiz, noAtual);

    while (true)
    {

        // Encontra o primeiro i no atual que key <= vetorChaves[i]
        int i = 0;
        while (i < noAtual->numChaves && key > noAtual->vetorChaves[i])
        {
            i++;
        }

        // Agora i é o índice do filho para onde descer OU
        // o índice da chave que pode ser igual a 'key'

        // se for folha, verifica se achou
        if (noAtual->ehFolha)
        {
            if (i < noAtual->numChaves && noAtual->vetorChaves[i] == key)
            {
                long apontadorDado = noAtual->vetorApontadores[i];
                delete noAtual;
                return apontadorDado; //Achou 
            }
            else
            {
                delete noAtual;
                return -1; // Não achou
            }
        }
        else // se nao for folha, desce
        {
            // Se a chave for EXATAMENTE igual a uma chave interna, na B+ Tree
            // o valor está na sub-árvore da DIREITA daquela chave.
            if (i < noAtual->numChaves && noAtual->vetorChaves[i] == key)
            {
                i++;
            }

            // 'i' é o índice do filho para onde descer.
            long filhoId = noAtual->vetorApontadores[i]; 

            // Reutiliza o apontador 'noAtual' para carregar o filho.
            lerNo(filhoId, noAtual); // atualiza o noAtual com o filho
        }
    }// while (true)
}
