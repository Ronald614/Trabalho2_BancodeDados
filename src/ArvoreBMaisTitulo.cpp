// Bibliotecas padrão necessárias
#include <iostream>           // Para imprimir entradas invalidas
#include <vector>             // Para std::vector usado em NoTitulo
#include <cstring>            // Para memcpy e memset
#include <stdexcept>          // Para exceções padrão usados no construtor
#include <math.h>             // Para floor (na definição de 'm')
#include <cmath> 
#include "GerenciaBlocos.hpp" // Gerenciamento de blocos em disco
#include "ArvoreBMaisTitulo.hpp"    // Declaração da classe BPlusTreeTitulo

/*
 * ==========================================================
 * Implementação dos Métodos da Classe BPlusTreeTitulo
 * ==========================================================
 */

// Construtor da árvore B+
BPlusTreeTitulo::BPlusTreeTitulo(const std::string &nomeArquivo, const size_t tamanhoBloco_arg)
    : tamanhoBloco(static_cast<int>(tamanhoBloco_arg)), 
      nomeArquivo(nomeArquivo),
      gerenciador(nomeArquivo, tamanhoBloco_arg),
      idRaiz(-1),
      totalBlocos(0)
{
    try
    {
        // se a arvore existe o arquivo tem algo no cabecalhoTitulo
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
            // Caso o arquivo nao exista, criar o cabecalhoTitulo
            this->idRaiz = -1;            // arvore vazia
            this->totalBlocos = 1;       // total de blocos para o cabecalhoTitulo / total de bloco para a classe

            // Escreve o cabecalhoTitulo no arquivo
            escreverCabecalho();
        }

        // Calcula 'm' depois de tudo estar definido.
        int tamMetadados = sizeof(bool) //eh_folha
                         + sizeof(int)  // numChaves
                         + sizeof(long); // proximo
        m = floor((this->tamanhoBloco - tamMetadados) / (sizeof(ChaveTitulo) + sizeof(long)));
        // m muito menor devido ao tamanho de uma string ser incomparavelmente maior
    }
    catch (const std::exception &e)
    {
        // Re-lança a exceção para que o programa que chamou o construtor saiba que algo deu errado
        throw;
    }
}

// Converte um objeto NoTitulo para um buffer de bytes
void BPlusTreeTitulo::serializaNo(const NoTitulo &NoTitulo, char *buffer)
{
    char *ptr = buffer;

    // Copia os campos de tamanho fixo
    memcpy(ptr, &NoTitulo.ehFolha, sizeof(bool));
    ptr += sizeof(bool);
    memcpy(ptr, &NoTitulo.numChaves, sizeof(int));
    ptr += sizeof(int);
    memcpy(ptr, &NoTitulo.proximo, sizeof(long));
    ptr += sizeof(long);

    //Copia o conteúdo do vector de chaves
    // .data() retorna um ponteiro para o array interno do vector
    memcpy(ptr, NoTitulo.vetorChaves.data(), NoTitulo.numChaves * sizeof(ChaveTitulo)); // agora char[300]
    ptr += NoTitulo.numChaves * sizeof(ChaveTitulo); // agora char[300]
    // Copia o conteúdo do vector de apontadores
    if (NoTitulo.ehFolha)
    {
        // Se é folha, contem M chaves e M apontadores de DADOS
        memcpy(ptr, NoTitulo.vetorApontadores.data(), NoTitulo.numChaves * sizeof(long));
        // (Opcional) Mover o ptr: ptr += NoTitulo.numChaves * sizeof(long);
    }
    else
    {
        // Se é interno, contém M-1 chaves e M apontadores de FILHOS
        memcpy(ptr, NoTitulo.vetorApontadores.data(), (NoTitulo.numChaves + 1) * sizeof(long));
        // (Opcional) Mover o ptr: ptr += (NoTitulo.numChaves + 1) * sizeof(long);
    }
}

// Converte um buffer de bytes de volta para um objeto NoTitulo
void BPlusTreeTitulo::deserializaNo(const char *buffer, NoTitulo &NoTitulo)
{
    const char *ptr = buffer;

    //Lê os campos de tamanho fixo
    memcpy(&NoTitulo.ehFolha, ptr, sizeof(bool));
    ptr += sizeof(bool);
    memcpy(&NoTitulo.numChaves, ptr, sizeof(int));
    ptr += sizeof(int);
    memcpy(&NoTitulo.proximo, ptr, sizeof(long));
    ptr += sizeof(long);

    // Lê as chave 
    NoTitulo.vetorChaves.resize(NoTitulo.numChaves); // ajusta o tamanho do vetor
    memcpy(NoTitulo.vetorChaves.data(), ptr, NoTitulo.numChaves * sizeof(ChaveTitulo));  // agora char[300]
    ptr += NoTitulo.numChaves * sizeof(ChaveTitulo); // agora char[300]

    // Lê os apontadores
    if (NoTitulo.ehFolha)
    {
        // Se é folha, contém 'numChaves' ponteiros de DADOS
        NoTitulo.vetorApontadores.resize(NoTitulo.numChaves);
        memcpy(NoTitulo.vetorApontadores.data(), ptr, NoTitulo.numChaves * sizeof(long));
    }
    else
    {
        // Se é interno, contem 'numChaves + 1' ponteiros de FILHOS
        NoTitulo.vetorApontadores.resize(NoTitulo.numChaves + 1);
        memcpy(NoTitulo.vetorApontadores.data(), ptr, (NoTitulo.numChaves + 1) * sizeof(long));
    }
}

// Escreve um nó no disco
void BPlusTreeTitulo::escreverNo(NoTitulo *NoTitulo)
{
    char *buffer = new char[tamanhoBloco];
    serializaNo(*NoTitulo, buffer);
    gerenciador.escreveBloco(NoTitulo->selfId, buffer);
    delete[] buffer;
}

// Lê um nó do disco e o desserializa
NoTitulo *BPlusTreeTitulo::lerNo(long blockNum, NoTitulo *NoTitulo)
{
    char *buffer = new char[tamanhoBloco];
    gerenciador.lerBloco(blockNum, buffer);
    deserializaNo(buffer, *NoTitulo);
    NoTitulo->selfId = blockNum;
    delete[] buffer;
    return NoTitulo;
}

//  Escreve o cabecalhoTitulo da árvore B+ no bloco 0 do arquivo de índice.
void BPlusTreeTitulo::escreverCabecalho()
{
    // Adiciona os metadados ao buffer e escreve no bloco 0.
    cabecalhoTitulo hdr;
    hdr.idRaiz = this->idRaiz;
    hdr.tamanhoBloco = this->tamanhoBloco;
    hdr.numBlocos = this->totalBlocos;

    // Aloca um buffer na heap para o bloco
    char *buffer = new char[tamanhoBloco];

    // Importante zera o buffer para não ter lixo além do cabecalhoTitulo
    memset(buffer, 0, tamanhoBloco);

    // Copia a struct cabecalhoTitulo para o buffer
    memcpy(buffer, &hdr, sizeof(cabecalhoTitulo));

    // Escreve o buffer no bloco 0 do arquivo
    gerenciador.escreveBloco(0, buffer);

    //Libera a memória alocada.
    delete[] buffer;
}

// Lê o cabecalhoTitulo da árvore B+ do bloco 0 do arquivo de índice.
void BPlusTreeTitulo::lerCabecalho()
{
    // 1. Aloca um buffer na heap para receber os dados.
    char *buffer = new char[tamanhoBloco];

    // 2. Lê o bloco 0 do disco para o buffer.
    gerenciador.lerBloco(0, buffer);

    // 3. Cria uma struct para receber a cópia.
    cabecalhoTitulo hdr;

    // 4. Copia os bytes do buffer para a struct.
    memcpy(&hdr, buffer, sizeof(cabecalhoTitulo));

    // 5. Libera a memória, pois os dados já estão seguros em 'hdr'.
    delete[] buffer;

    // 6. Atualiza os atributos da classe.
    this->idRaiz = hdr.idRaiz;
    this->tamanhoBloco = hdr.tamanhoBloco;
    this->totalBlocos = hdr.numBlocos;
}

// Retorna um novo ID para um nó baseado no total de blocos
long BPlusTreeTitulo::getNovoId()
{
    long id = this->totalBlocos; // o novo id é o total de blocos atual
    // o total de blocos base é 1 (bloco do cabecalhoTitulo) gravado no construtor
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
void BPlusTreeTitulo::splitChild(NoTitulo *parent, int irmaoIndex)
{

    // 1. Cria o novo "irmão" (novoIrmao) e aloca um bloco para ele no disco
    NoTitulo *novoIrmao = new NoTitulo();
    novoIrmao->selfId = this->getNovoId(); // atribui um novo id para o novo nó

    // 2. Carrega o filho lotado ('irmao') do disco
    long idFilho = parent->vetorApontadores[irmaoIndex];
    NoTitulo *irmao = new NoTitulo();
    lerNo(idFilho, irmao);

    // 3. Define o status do novo irmão (folha ou não)
    novoIrmao->ehFolha = irmao->ehFolha;

    // 4. Calcula o "ponto do meio" para o split
    int indiceMeio;
    ChaveTitulo chavePromovida; // A chave que vai subir para o 'parent'

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
 * @param NoTitulo O nó atual (JÁ EM MEMÓRIA) que estamos inspecionando.
 * @param key A chave que queremos inserir.
 * @param dataPointer O ponteiro de dado associado.
 **/
void BPlusTreeTitulo::insertNonFull(NoTitulo *noAtual, const ChaveTitulo& key, long dataPointer)
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
        NoTitulo *irmaoNo = new NoTitulo();
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
void BPlusTreeTitulo::insert(const ChaveTitulo& key, long dataPointer){

    // CASO 1: Árvore vazia. Eu só crio a 'raiz' como folha
    // e coloco a chave e o apontador nela.
    if (idRaiz == -1){
        NoTitulo *primeiraRaiz = new NoTitulo(true); // Cria a raiz, que também é uma folha.
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
        NoTitulo *raizInicial = new NoTitulo();
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
            NoTitulo *novaRaiz = new NoTitulo(false); // Nova raiz não eh folha (porque vai ter filhos)
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
 * @brief Busca por uma ChaveTitulo na árvore.
 *
 * Desce a árvore até a folha onde a chave deveria estar.
 * Em seguida, varre a(s) folha(s) para encontrar todas as ocorrências
 * da chave e retorna um vetor com os dataPointers correspondentes.
 * * @param key A ChaveTitulo a ser buscada.
 * @return std::vector<long> Um vetor contendo todos os dataPointers 
 * associados à chave. Retorna um vetor vazio se a chave não for encontrada.
 */
std::vector<long> BPlusTreeTitulo::search(const ChaveTitulo& key)
{
    std::vector<long> resultados; // Vetor para armazenar os dataPointers encontrados

    if (idRaiz == -1)
    {
        // Árvore vazia, retorna vetor vazio
        return resultados; 
    }

    NoTitulo *noAtual = new NoTitulo();
    lerNo(idRaiz, noAtual);

    // --- 1. Desce até a folha correta ---
    while (!noAtual->ehFolha)
    {
        // Encontra o índice do filho para onde descer
        int i = 0;
        // Usa o operador >= para encontrar o primeiro ramo >= key
        while (i < noAtual->numChaves && key >= noAtual->vetorChaves[i]) 
        {
            // Nota: Se key == vetorChaves[i] em nó interno, descemos para a direita (i+1)
            //       Se key > vetorChaves[i], continuamos procurando
            //       Se key < vetorChaves[i], paramos e descemos por i
             if (key == noAtual->vetorChaves[i]) {
                 // Em B+ interna, igualdade significa descer pelo ponteiro à direita
                 i++; 
                 break; // Sai do while, 'i' está correto
             }
             if (key > noAtual->vetorChaves[i]) {
                  i++;
             } else {
                 break; // key < vetorChaves[i], desce por i
             }
        }
        // 'i' é o índice do filho para onde descer.
        long filhoId = noAtual->vetorApontadores[i]; 
        lerNo(filhoId, noAtual); // Carrega o filho
    }

    // --- 2. Varre a(s) folha(s) a partir do ponto encontrado ---
    // 'noAtual' agora é a primeira folha onde a 'key' pode estar.
    
    // Encontra o primeiro índice 'i' na folha onde a chave é >= key
    int i = 0;
    while (i < noAtual->numChaves && key > noAtual->vetorChaves[i]) {
        i++;
    }

    // Agora, varre a partir do índice 'i' e continua pelas folhas seguintes
    bool continuarVarrendo = true;
    while (continuarVarrendo) {
        // Varre o nó folha atual a partir do índice 'i'
        while (i < noAtual->numChaves) {
            // Verifica se a chave na posição atual é a que buscamos
            if (noAtual->vetorChaves[i] == key) {
                // Achou uma ocorrência! Adiciona o dataPointer ao resultado.
                resultados.push_back(noAtual->vetorApontadores[i]);
                i++; // Continua verificando a próxima chave na mesma folha
            } else {
                // Encontrou uma chave diferente, para a varredura total.
                continuarVarrendo = false; 
                break; // Sai do loop interno (while i < numChaves)
            }
        }

        // Se saiu do loop interno, verifica se precisa continuar na próxima folha
        if (!continuarVarrendo) {
            break; // Sai do loop externo (while continuarVarrendo)
        }

        // Chegou ao fim da folha atual. Vai para a próxima?
        if (noAtual->proximo != -1) {
            lerNo(noAtual->proximo, noAtual); // Carrega a próxima folha
            i = 0; // Começa a varredura do início da nova folha
        } else {
            // Não há mais folhas, termina a busca.
            continuarVarrendo = false;
        }
    }

    // Libera a memória do último nó carregado
    delete noAtual;
    
    // Retorna o vetor de resultados (pode estar vazio)
    return resultados; 
}

// ==========================================================
// NOVAS FUNÇÕES PARA REMOÇÃO (Tipos Corrigidos)
// ==========================================================

// Função pública que inicia a remoção
// Assinatura já estava correta, mas a chamada recursiva precisa ser corrigida.
void BPlusTreeTitulo::remove(const ChaveTitulo& key, long dataPointer) {
    if (idRaiz == -1) {
        std::cout << "Árvore vazia. Não é possível remover." << std::endl;
        return;
    }

    // Carrega a raiz
    NoTitulo* noRaiz = new NoTitulo();
    lerNo(idRaiz, noRaiz);

    // Chama a função recursiva de remoção, passando dataPointer
    removeRecursivo(noRaiz, key, dataPointer); // <-- CORRIGIDO AQUI

    // --- Tratamento especial para a raiz ---
    if (noRaiz->numChaves == 0) {
        if (noRaiz->ehFolha) {
            idRaiz = -1; 
        } else {
            idRaiz = noRaiz->vetorApontadores[0];
        }
        escreverCabecalho(); 
    }
    // --------------------------------------

    delete noRaiz; // Libera a memória do nó raiz carregado
}

// Encontra o índice da chave 'key' em 'no'
// Corrige o tipo do parâmetro 'key'
int BPlusTreeTitulo::findKeyIndex(NoTitulo* no, const ChaveTitulo& key) { // <-- CORRIGIDO AQUI
    int indice = 0;
    // A comparação '<' funciona devido aos operadores definidos em ChaveTitulo
    while (indice < no->numChaves && no->vetorChaves[indice] < key) { 
        ++indice;
    }
    return indice;
}

// Função recursiva principal para remoção (CORRIGIDA)
// Atualiza a chamada para removeFromInternal
void BPlusTreeTitulo::removeRecursivo(NoTitulo* noAtual, const ChaveTitulo& key, long dataPointer) { 
    int indiceChave = findKeyIndex(noAtual, key);

    // --- CASO 1: A chave (ou uma chave igual) está no nó ATUAL ---
    if (indiceChave < noAtual->numChaves && noAtual->vetorChaves[indiceChave] == key) { 
        if (noAtual->ehFolha) {
            removeFromLeaf(noAtual, indiceChave, dataPointer); 
        } else {
            // Passa o dataPointer original para removeFromInternal
            removeFromInternal(noAtual, indiceChave, dataPointer); // <-- CORRIGIDO AQUI
        }
    } 
    // --- CASO 2: A chave NÃO está no nó atual ---
    else {
        if (noAtual->ehFolha) {
            std::cout << "Chave (Título) não encontrada na árvore para remoção." << std::endl; 
            return;
        }

        // --- Subcaso 2.1: Nó é interno, desce para o filho apropriado ---
        
        long idFilho = noAtual->vetorApontadores[indiceChave];
        NoTitulo* noFilho = new NoTitulo();
        try {
            lerNo(idFilho, noFilho);
        } catch (const std::exception& e) {
            delete noFilho; 
            throw std::runtime_error("Erro ao tentar ler filho ID " + std::to_string(idFilho) + " em removeRecursivo: " + e.what()); 
        }

        int minChaves;
        if (noFilho->ehFolha) {
             minChaves = m / 2; 
        } else {
             minChaves = (m + 1) / 2 - 1; 
        }

        if (noFilho->numChaves <= minChaves) { 
            
            delete noFilho; 
            noFilho = nullptr; 

            fillNode(noAtual, indiceChave); 

            // Reinicia a recursão a partir do pai, passando dataPointer
            removeRecursivo(noAtual, key, dataPointer); 
            
            return; 

        } else {
             // O filho tem chaves suficientes, desce recursivamente passando dataPointer
             removeRecursivo(noFilho, key, dataPointer); 
             
             if (noFilho != nullptr) {
                delete noFilho; 
             }
        }
       
    }
}

// Remove a chave do nó folha
// Adiciona dataPointer para encontrar a entrada correta
void BPlusTreeTitulo::removeFromLeaf(NoTitulo* folha, int keyIndex, long dataPointer) { // <-- CORRIGIDO AQUI
    // Encontra o índice exato do par (key, dataPointer) a partir de keyIndex
    int indiceParaRemover = -1;
    for (int i = keyIndex; i < folha->numChaves; ++i) {
        // Verifica se a chave ainda é a mesma E se o dataPointer bate
        if (folha->vetorChaves[i] == folha->vetorChaves[keyIndex] && folha->vetorApontadores[i] == dataPointer) {
             indiceParaRemover = i;
             break;
        }
        // Se a chave mudou, o par não existe
        if (!(folha->vetorChaves[i] == folha->vetorChaves[keyIndex])) {
            break;
        }
    }

    if (indiceParaRemover != -1) {
        folha->vetorChaves.erase(folha->vetorChaves.begin() + indiceParaRemover);
        folha->vetorApontadores.erase(folha->vetorApontadores.begin() + indiceParaRemover);
        folha->numChaves--;
        escreverNo(folha);
    } else {
        std::cout << "Par (Chave, DataPointer=" << dataPointer << ") não encontrado na folha para remoção." << std::endl;
    }
}

// Remove a chave do nó interno
// Adiciona dataPointer como parâmetro e usa o DP real na recursão
void BPlusTreeTitulo::removeFromInternal(NoTitulo* interno, int keyIndex, long dataPointerOriginal) { // <-- Adicionado dataPointerOriginal
    ChaveTitulo chave = interno->vetorChaves[keyIndex]; // Chave a ser substituída

    long idFilhoEsquerdo = interno->vetorApontadores[keyIndex];
    long idFilhoDireito = interno->vetorApontadores[keyIndex + 1];
    NoTitulo* filhoEsquerdo = new NoTitulo();
    NoTitulo* filhoDireito = new NoTitulo();
    // Adicionar try-catch aqui seria bom para robustez
    lerNo(idFilhoEsquerdo, filhoEsquerdo);
    lerNo(idFilhoDireito, filhoDireito);

    int minChaves;
     if (filhoEsquerdo->ehFolha) { 
          minChaves = m / 2; 
     } else {
          minChaves = (m + 1) / 2 -1; 
     }

    // --- CASO 2a: O filho esquerdo tem chaves suficientes (> minChaves) ---
    if (filhoEsquerdo->numChaves > minChaves) {
        // Obtém o par (chave, dataPointer) do predecessor
        std::pair<ChaveTitulo, long> predecessorPar = getPred(interno, keyIndex);
        ChaveTitulo predecessorChave = predecessorPar.first;
        long predecessorDataPointer = predecessorPar.second;

        // Substitui a chave no nó interno pela chave predecessora
        interno->vetorChaves[keyIndex] = predecessorChave;
        escreverNo(interno); 

        // Recursivamente remove o predecessor (chave E dataPointer) de onde ele veio
        removeRecursivo(filhoEsquerdo, predecessorChave, predecessorDataPointer); // <-- Usa DP real
    }
    // --- CASO 2b: O filho direito tem chaves suficientes (> minChaves) ---
    else if (filhoDireito->numChaves > minChaves) {
        // Obtém o par (chave, dataPointer) do sucessor
        std::pair<ChaveTitulo, long> sucessorPar = getSucc(interno, keyIndex);
        ChaveTitulo sucessorChave = sucessorPar.first;
        long sucessorDataPointer = sucessorPar.second;

        // Substitui a chave no nó interno pela chave sucessora
        interno->vetorChaves[keyIndex] = sucessorChave;
        escreverNo(interno); 

        // Recursivamente remove o sucessor (chave E dataPointer) de onde ele veio
        removeRecursivo(filhoDireito, sucessorChave, sucessorDataPointer); // <-- Usa DP real
    }
    // --- CASO 2c: Ambos os filhos estão no limite mínimo (minChaves) ---
    else {
        // Salva a chave e o dataPointer originais antes do merge
        ChaveTitulo chaveOriginal = interno->vetorChaves[keyIndex]; 
        // dataPointerOriginal já é passado como parâmetro

        // Funde filhoEsquerdo, chave[keyIndex] do pai, e filhoDireito.
        // A função merge remove a chave[keyIndex] do pai.
        merge(interno, keyIndex); 

        // Após o merge, a chave original (que estava no pai ou no filho direito)
        // agora está no nó filhoEsquerdo (que foi expandido).
        // Precisamos chamar removeRecursivo neste nó fundido para remover o par (chave, dataPointer) original.
        
        // Recarrega o filho esquerdo, pois 'merge' deletou os ponteiros internos
        // e modificou o nó no disco.
        NoTitulo* filhoEsquerdoMerged = new NoTitulo();
        // Assume que lerNo lança exceção em caso de erro
        lerNo(idFilhoEsquerdo, filhoEsquerdoMerged); 

        // Chama a recursão no nó fundido para remover o par original
        removeRecursivo(filhoEsquerdoMerged, chaveOriginal, dataPointerOriginal); 

        delete filhoEsquerdoMerged; // Libera o nó recarregado
    }

    delete filhoEsquerdo;
    delete filhoDireito;
}

// Encontra a maior chave E o dataPointer correspondente na subárvore esquerda
// Tipo de retorno modificado para std::pair
std::pair<ChaveTitulo, long> BPlusTreeTitulo::getPred(NoTitulo* no, int index) { 
    long idAtual = no->vetorApontadores[index];
    NoTitulo* noAtual = new NoTitulo();
    lerNo(idAtual, noAtual);

    // Desce pela direita até encontrar uma folha
    while (!noAtual->ehFolha) {
        long idProximo = noAtual->vetorApontadores[noAtual->numChaves]; // Último ponteiro
        lerNo(idProximo, noAtual);
    }

    // A última chave e o último dataPointer da folha são o predecessor
    ChaveTitulo predecessorChave = noAtual->vetorChaves.back(); // Última chave
    long predecessorDataPointer = noAtual->vetorApontadores.back(); // Último dataPointer

    delete noAtual;
    return {predecessorChave, predecessorDataPointer}; // Retorna o par
}

// Encontra a menor chave E o dataPointer correspondente na subárvore direita
// Tipo de retorno modificado para std::pair
std::pair<ChaveTitulo, long> BPlusTreeTitulo::getSucc(NoTitulo* no, int index) { 
    long idAtual = no->vetorApontadores[index + 1];
    NoTitulo* noAtual = new NoTitulo();
    lerNo(idAtual, noAtual);

    // Desce pela esquerda até encontrar uma folha
    while (!noAtual->ehFolha) {
        long idProximo = noAtual->vetorApontadores[0]; // Primeiro ponteiro
        lerNo(idProximo, noAtual);
    }

    // A primeira chave e o primeiro dataPointer da folha são o sucessor
    ChaveTitulo sucessorChave = noAtual->vetorChaves.front(); // Primeira chave
    long sucessorDataPointer = noAtual->vetorApontadores.front(); // Primeiro dataPointer

    delete noAtual;
    return {sucessorChave, sucessorDataPointer}; // Retorna o par
}

// --- Funções para Lidar com Underflow ---
// (Estas funções precisam manipular ChaveTitulo ao mover chaves)

void BPlusTreeTitulo::fillNode(NoTitulo* pai, int indexFilho) {
    // Tenta emprestar do irmão esquerdo primeiro
    if (indexFilho != 0) { 
        long idIrmaoAnterior = pai->vetorApontadores[indexFilho - 1];
        NoTitulo* irmaoAnterior = new NoTitulo();
        lerNo(idIrmaoAnterior, irmaoAnterior);

         int minChaves;
         if (irmaoAnterior->ehFolha) { 
              minChaves = m / 2; 
         } else {
              minChaves = (m + 1) / 2 -1; 
         }

        if (irmaoAnterior->numChaves > minChaves) {
            borrowFromPrev(pai, indexFilho); 
            delete irmaoAnterior;
            return; 
        }
        delete irmaoAnterior; 
    }

    // Tenta emprestar do irmão direito
    if (indexFilho != pai->numChaves) { 
        long idIrmaoProximo = pai->vetorApontadores[indexFilho + 1];
        NoTitulo* irmaoProximo = new NoTitulo();
        lerNo(idIrmaoProximo, irmaoProximo);

         int minChaves;
         if (irmaoProximo->ehFolha) { 
              minChaves = m / 2; 
         } else {
              minChaves = (m + 1) / 2 -1; 
         }


        if (irmaoProximo->numChaves > minChaves) {
            borrowFromNext(pai, indexFilho); 
            delete irmaoProximo;
            return; 
        }
        delete irmaoProximo; 
    }

    // Se não pode emprestar de nenhum dos lados, faz merge
    if (indexFilho != pai->numChaves) {
        merge(pai, indexFilho); 
    } else {
        // Se for o último filho, faz merge com o irmão esquerdo (índice indexFilho - 1)
        merge(pai, indexFilho - 1); 
    }
}

// Empresta uma chave do irmão esquerdo ('indexFilho - 1') para o filho ('indexFilho')
// Precisa usar ChaveTitulo ao mover chaves
void BPlusTreeTitulo::borrowFromPrev(NoTitulo* pai, int indexFilho) {
    long idFilho = pai->vetorApontadores[indexFilho];
    long idIrmaoAnterior = pai->vetorApontadores[indexFilho - 1];
    NoTitulo* noFilho = new NoTitulo();
    NoTitulo* irmaoAnterior = new NoTitulo();
    lerNo(idFilho, noFilho);
    lerNo(idIrmaoAnterior, irmaoAnterior);

    if (!noFilho->ehFolha) {
        // Move chave do pai para filho
        noFilho->vetorChaves.insert(noFilho->vetorChaves.begin(), pai->vetorChaves[indexFilho - 1]);
        // Move ponteiro do irmão para filho
        noFilho->vetorApontadores.insert(noFilho->vetorApontadores.begin(), irmaoAnterior->vetorApontadores.back());
        irmaoAnterior->vetorApontadores.pop_back();
        // Move chave do irmão para pai
        pai->vetorChaves[indexFilho - 1] = irmaoAnterior->vetorChaves.back(); // <-- ChaveTitulo sendo copiada
        irmaoAnterior->vetorChaves.pop_back();

    } 
    else { // Caso folha B+
        // Copia chave/ponteiro do irmão para filho
        noFilho->vetorChaves.insert(noFilho->vetorChaves.begin(), irmaoAnterior->vetorChaves.back()); // <-- ChaveTitulo
        noFilho->vetorApontadores.insert(noFilho->vetorApontadores.begin(), irmaoAnterior->vetorApontadores.back()); // <-- long
        irmaoAnterior->vetorChaves.pop_back();
        irmaoAnterior->vetorApontadores.pop_back();
        // Atualiza chave no pai com a nova primeira chave da folha que recebeu
        pai->vetorChaves[indexFilho - 1] = noFilho->vetorChaves[0]; // <-- ChaveTitulo
    }

    noFilho->numChaves++;
    irmaoAnterior->numChaves--;

    escreverNo(pai);
    escreverNo(noFilho);
    escreverNo(irmaoAnterior);

    delete noFilho;
    delete irmaoAnterior;
}

// Empresta uma chave do irmão direito ('indexFilho + 1') para o filho ('indexFilho')
// Precisa usar ChaveTitulo ao mover chaves
void BPlusTreeTitulo::borrowFromNext(NoTitulo* pai, int indexFilho) {
    long idFilho = pai->vetorApontadores[indexFilho];
    long idIrmaoProximo = pai->vetorApontadores[indexFilho + 1];
    NoTitulo* noFilho = new NoTitulo();
    NoTitulo* irmaoProximo = new NoTitulo();
    lerNo(idFilho, noFilho);
    lerNo(idIrmaoProximo, irmaoProximo);

    if (!noFilho->ehFolha) {
        // Move chave do pai para filho
        noFilho->vetorChaves.push_back(pai->vetorChaves[indexFilho]); // <-- ChaveTitulo
        // Move ponteiro do irmão para filho
        noFilho->vetorApontadores.push_back(irmaoProximo->vetorApontadores.front());
        irmaoProximo->vetorApontadores.erase(irmaoProximo->vetorApontadores.begin());
        // Move chave do irmão para pai
        pai->vetorChaves[indexFilho] = irmaoProximo->vetorChaves.front(); // <-- ChaveTitulo
        irmaoProximo->vetorChaves.erase(irmaoProximo->vetorChaves.begin());
    }
    else { // Caso folha B+
        // Copia chave/ponteiro do irmão para filho
        noFilho->vetorChaves.push_back(irmaoProximo->vetorChaves.front()); // <-- ChaveTitulo
        noFilho->vetorApontadores.push_back(irmaoProximo->vetorApontadores.front()); // <-- long
        irmaoProximo->vetorChaves.erase(irmaoProximo->vetorChaves.begin());
        irmaoProximo->vetorApontadores.erase(irmaoProximo->vetorApontadores.begin());
        // Atualiza chave no pai com a nova primeira chave do irmão (agora menor)
        pai->vetorChaves[indexFilho] = irmaoProximo->vetorChaves[0]; // <-- ChaveTitulo
    }

    noFilho->numChaves++;
    irmaoProximo->numChaves--;

    escreverNo(pai);
    escreverNo(noFilho);
    escreverNo(irmaoProximo);

    delete noFilho;
    delete irmaoProximo;
}

// Funde o filho direito ('indexFilho + 1') com o filho esquerdo ('indexFilho')
// Precisa usar ChaveTitulo ao mover chaves
void BPlusTreeTitulo::merge(NoTitulo* pai, int indexFilho) {
    long idFilhoEsquerdo = pai->vetorApontadores[indexFilho];
    long idFilhoDireito = pai->vetorApontadores[indexFilho + 1];
    NoTitulo* filhoEsquerdo = new NoTitulo();
    NoTitulo* filhoDireito = new NoTitulo();
    lerNo(idFilhoEsquerdo, filhoEsquerdo);
    lerNo(idFilhoDireito, filhoDireito);

    // Se for nó interno, a chave do pai desce para o meio
    if (!filhoEsquerdo->ehFolha) { 
        // Desce a chave do pai
        filhoEsquerdo->vetorChaves.push_back(pai->vetorChaves[indexFilho]); // <-- ChaveTitulo

        // Copia chaves do irmão direito
        filhoEsquerdo->vetorChaves.insert(filhoEsquerdo->vetorChaves.end(), 
                                      filhoDireito->vetorChaves.begin(), 
                                      filhoDireito->vetorChaves.end()); // <-- Vetor de ChaveTitulo

        // Copia ponteiros do irmão direito
        filhoEsquerdo->vetorApontadores.insert(filhoEsquerdo->vetorApontadores.end(), 
                                          filhoDireito->vetorApontadores.begin(), 
                                          filhoDireito->vetorApontadores.end()); // <-- Vetor de long
    }
    // Se for nó folha, apenas concatena (chave do pai não desce)
    else {
        // Copia chaves do irmão direito
        filhoEsquerdo->vetorChaves.insert(filhoEsquerdo->vetorChaves.end(), 
                                      filhoDireito->vetorChaves.begin(), 
                                      filhoDireito->vetorChaves.end()); // <-- Vetor de ChaveTitulo
        // Copia ponteiros de dados do irmão direito                              
        filhoEsquerdo->vetorApontadores.insert(filhoEsquerdo->vetorApontadores.end(), 
                                          filhoDireito->vetorApontadores.begin(), 
                                          filhoDireito->vetorApontadores.end()); // <-- Vetor de long
        
        // Atualiza a lista ligada de folhas
        filhoEsquerdo->proximo = filhoDireito->proximo;
    }

    // Atualiza contador de chaves do filho esquerdo
    // Adiciona chaves do filho direito + 1 (chave do pai) se for interno
    filhoEsquerdo->numChaves += filhoDireito->numChaves + (filhoEsquerdo->ehFolha ? 0 : 1);

    // Remove chave e ponteiro do pai
    pai->vetorChaves.erase(pai->vetorChaves.begin() + indexFilho); // <-- Remove ChaveTitulo
    pai->vetorApontadores.erase(pai->vetorApontadores.begin() + indexFilho + 1); // <-- Remove long
    pai->numChaves--;

    // Escreve pai e filho esquerdo modificados
    escreverNo(pai);
    escreverNo(filhoEsquerdo);
    
    // O bloco do filho direito agora está órfão (não é mais referenciado)
    // Uma implementação completa adicionaria idFilhoDireito a uma lista livre.

    // Libera memória dos ponteiros carregados
    delete filhoEsquerdo;
    delete filhoDireito;
}