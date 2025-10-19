/*
 * ==========================================================
 * Programa em C++ para Implementar uma Árvore B+ (B+ Tree)
 * ==========================================================
 */

// Bibliotecas padrão necessárias
#include <algorithm> // Para funções como std::lower_bound (útil na busca)
#include <iostream>  // Para std::cout (impressão na tela)
#include <vector>    // Para std::vector (nossas listas dinâmicas)

/*
 * [Sua Nota] Atalho para o namespace padrão.
 * Sem esta linha, teríamos que escrever std::vector, std::cout, etc.
 *
 * [Nota de Clean Code] Isso é ótimo para arquivos .cpp, mas evite
 * usar "using namespace" em arquivos de cabeçalho (.h)
 * para não "poluir" o código de quem incluir seu arquivo.
 */
using namespace std;

/**
 * @brief Define uma classe genérica de Árvore B+.
 *
 * [Sua Nota] 'template <typename T>' define T como um tipo genérico.
 * É parecido com a ideia do 'void*' em C, mas muito mais seguro,
 * pois o C++ verifica os tipos em tempo de compilação.
 *
 * @tparam T O tipo de dado que as chaves (keys) irão armazenar (int, string, etc.)
 */
template <typename T>
class BPlusTree {
// 'public:' significa que tudo abaixo pode ser acessado de fora da classe
public:
    /**
     * @brief [Sua Nota] Estrutura interna que define um Nó da árvore.
     */
    struct Node {
        // [Sua Nota] Valor booleano que indica se o nó é uma folha
        bool isLeaf;
        // [Sua Nota] Vetor de chaves, usando o tipo genérico T
        vector<T> keys;
        // [Sua Nota] Vetor de ponteiros para os nós filhos
        vector<Node*> children;
        // [Sua Nota] Ponteiro para o próximo nó folha (para rangeQuery)
        Node* next;

        /**
         * @brief Construtor do Nó.
         * [Sua Nota] Garante que o nó seja inicializado corretamente
         * para evitar "lixo de memória".
         *
         * @param leaf Define se o nó deve ser criado como folha (padrão: false).
         */
        Node(bool leaf = false)
            // [Sua Nota] 'isLeaf(leaf)' inicializa o membro 'isLeaf' com o parâmetro 'leaf'.
            : isLeaf(leaf),
            // [Sua Nota] 'next(nullptr)' inicializa o ponteiro 'next' como nulo.
              next(nullptr)
        {
            // O corpo do construtor está vazio, pois tudo foi feito
            // na lista de inicialização (acima).
        }
    };

    /*
     * ==========================================
     * Membros (Variáveis) da Classe BPlusTree
     * ==========================================
     */

    // [Sua Nota] 'root' é o ponteiro para o nó raiz, o início da árvore.
    Node* root;
    
    /*
     * [Sua Nota] 't' é o grau mínimo da árvore.
     * Ele define as regras de "lotação" do nó:
     * - Mínimo de chaves: t - 1
     * - Máximo de chaves: 2*t - 1
     * - Mínimo de filhos: t
     * - Máximo de filhos: 2*t
     */
    int t;

    /*
     * ==========================================
     * Funções "Helper" (Auxiliares Internas)
     * ==========================================
     * Estas são as funções que fazem o trabalho pesado.
     * Elas são chamadas pelas funções públicas (insert, remove, etc.)
     */

    // Função para dividir um nó filho que está cheio
    void splitChild(Node* parent, int index, Node* child);

    // Função para inserir uma chave em um nó que *não* está cheio
    void insertNonFull(Node* node, T key);

    // Função recursiva para remover uma chave
    void remove(Node* node, T key);

    // Funções de balanceamento (usadas na remoção)
    void borrowFromPrev(Node* node, int index);
    void borrowFromNext(Node* node, int index);
    void merge(Node* node, int index);

    // Função recursiva para imprimir a árvore
    void printTree(Node* node, int level);

/*
 * ==========================================
 * Interface Pública
 * ==========================================
 * [Sua Nota] Estas são as funções que o usuário final vai chamar.
 * Elas escondem a complexidade das funções auxiliares.
 */
public:
    /**
     * @brief Construtor da Árvore B+.
     * [Sua Nota] Recebe o grau mínimo como parâmetro obrigatório.
     *
     * @param degree O grau mínimo (t) da árvore.
     */
    BPlusTree(int degree)
        // [Sua Nota] Inicializa a raiz ('root') como nula, pois a árvore começa vazia.
        : root(nullptr),
        // [Sua Nota] Armazena o 'degree' na variável interna 't'.
          t(degree)
    {
    }

    // Funções públicas principais
    void insert(T key);
    bool search(T key);
    void remove(T key);
    vector<T> rangeQuery(T lower, T upper);
    void printTree();

}; // <-- [Correção] Fim da definição da classe BPlusTree

/*
 * ==========================================================
 * Implementação das Funções
 * ==========================================================
 */

/**
 * @brief Divide um nó 'child' lotado.
 * [Sua Nota] 'parent' é o nó pai, 'index' é a posição do 'child'
 * no vetor de filhos do pai, e 'child' é o nó lotado.
 */
template <typename T>
void BPlusTree<T>::splitChild(Node* parent, int index,
                              Node* child)
{
    // [Sua Nota] Cria um novo nó "irmão", com o mesmo status (folha ou não)
    // do nó que será dividido.
    Node* newChild = new Node(child->isLeaf);

    // [Sua Nota] Insere o 'newChild' no vetor de filhos do 'parent',
    // na posição 'index + 1' (logo ao lado do 'child' original).
    parent->children.insert(
        parent->children.begin() + index + 1, newChild);

    // [Sua Nota] "Promove" a chave do meio (t-1) do 'child' lotado
    // para o 'parent', na posição 'index'.
    parent->keys.insert(parent->keys.begin() + index,
                        child->keys[t - 1]);

    // [Sua Nota] Copia a "metade direita" das chaves do 'child'
    // original para o 'newChild'.
    newChild->keys.assign(child->keys.begin() + t,
                          child->keys.end());

    // [Sua Nota] Reduz o tamanho do 'child' original para que ele
    // contenha apenas a "metade esquerda" das chaves.
    child->keys.resize(t - 1);

    // [Sua Nota] Se o 'child' NÃO for uma folha (for um nó interno),
    // temos que dividir também o vetor de 'children' dele.
    if (!child->isLeaf) {
        // [Sua Nota] O 'newChild' fica com a "metade direita" dos filhos.
        newChild->children.assign(child->children.begin() + t,
                                  child->children.end());
        // [Sua Nota] O 'child' original fica com a "metade esquerda" dos filhos.
        child->children.resize(t);
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
 * @param key A chave (do tipo genérico T) que queremos inserir.
 */
template <typename T>
void BPlusTree<T>::insertNonFull(Node* node, T key)
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
        // A "MÁGICA" DA B+ TREE: Dividir antes de descer
        // ==========================================================
        // [Sua Nota] Verificamos se a "gaveta" (filho) para onde
        // vamos descer está LOTADA. (2*t - 1 é o n° máximo de chaves).
        if (node->children[i]->keys.size() == 2 * t - 1) {
            
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
        
        // [Sua Nota] Agora temos CERTEZA que o filho 'children[i]'
        // tem espaço. Chamamos a mesma função de novo, mas
        // um nível abaixo (no filho que escolhemos).
        insertNonFull(node->children[i], key);
    }
}

/**
 * @brief Função de balanceamento (usada na remoção).
 * [Sua Nota] Pega uma chave "emprestada" do IRMÃO ANTERIOR (da esquerda).
 * Isso é uma "rotação de chaves".
 *
 * Cenário: 'node->children[index]' está com poucas chaves (underflow).
 * 'node->children[index - 1]' (o irmão) tem chaves sobrando.
 *
 * @param node O nó PAI de ambos os filhos.
 * @param index O índice do nó "pobre" (que precisa de uma chave).
 */

template <typename T>
void BPlusTree<T>::borrowFromPrev(Node* node, int index)
{
    // O nó "pobre" (que precisa da chave)
    Node* child = node->children[index];
    // O nó "rico" (o irmão da esquerda, que vai doar a chave)
    Node* sibling = node->children[index - 1];

    // --- A Rotação de Chaves 🔄 ---

    // [Sua Nota] Passo 1: O filho "pobre" ('child') pega a chave
    // "separadora" que está no PAI ('node') e a coloca
    // como sua *primeira* chave.
    child->keys.insert(child->keys.begin(),
                       node->keys[index - 1]);

    // [Sua Nota] Passo 2: O PAI ('node') pega a *última* chave
    // do irmão "rico" ('sibling') para preencher o "buraco".
    node->keys[index - 1] = sibling->keys.back();

    // [Sua Nota] Passo 3: O irmão "rico" ('sibling') remove a
    // última chave (que ele acabou de doar para o pai).
    sibling->keys.pop_back();

    // [Sua Nota] Se não forem folhas (forem nós internos),
    // temos que rotacionar os *ponteiros de filhos* também!
    if (!child->isLeaf) {
        // [Sua Nota] O 'child' "pobre" adota o *último* filho
        // do 'sibling' "rico".
        child->children.insert(child->children.begin(),
                               sibling->children.back());
        // [Sua Nota] O 'sibling' "rico" remove o ponteiro do filho
        // que ele acabou de doar.
        sibling->children.pop_back();
    }
}