// C++ Program to Implement B+ Tree
#include <algorithm>
#include <iostream>
#include <vector>
using namespace std;

// B plus tree class
template <typename T> class BPlusTree {
public:
    // structure to create a node
    // Minha struct de Nó. O 'next' é a chave da B+ Tree,
    // ele que liga uma folha na outra (a "corrente" 🔗)
    struct Node {
        bool isLeaf;
        vector<T> keys;
        vector<Node*> children;
        Node* next;

        // Construtor do Nó. Garante que todo nó novo
        // já comece com 'next' nulo e saiba se é folha ou não,
        // pra evitar lixo de memória.
        Node(bool leaf = false)
            : isLeaf(leaf)
            , next(nullptr)
        {
        }
    };

    // 'root' é o ponteiro pro início da árvore
    Node* root;
    // 't' é o "grau mínimo". É a regra principal.
    // N° MÁXIMO de chaves = 2*t - 1
    // N° MÍNIMO de chaves = t - 1
    int t;

    // ----- Funções Auxiliares (o "trabalho sujo") -----
    void splitChild(Node* parent, int index, Node* child);
    void insertNonFull(Node* node, T key);
    void remove(Node* node, T key);
    void borrowFromPrev(Node* node, int index);
    void borrowFromNext(Node* node, int index);
    void merge(Node* node, int index);
    void printTree(Node* node, int level);

public:
    // Construtor da Árvore. Começa com 'root' nula e
    // define qual é o 't' (a "regra de lotação").
    BPlusTree(int degree): root(nullptr), t(degree){}

    // ----- Funções Públicas (o "menu" que o usuário usa) -----
    void insert(T key);
    bool search(T key);
    void remove(T key);
    vector<T> rangeQuery(T lower, T upper);
    void printTree();
};

/*
 * ==========================================================
 * IMPLEMENTAÇÃO DAS FUNÇÕES
 * ==========================================================
 */

/**
 * @brief (splitChild) Essa é uma das mais importantes.
 *
 * Lógica: Quando um nó filho ('child') tá lotado (2*t - 1 chaves)
 * e a gente tenta inserir mais, essa função é chamada.
 */
template <typename T>
void BPlusTree<T>::splitChild(Node* parent, int index,
                              Node* child)
{
    // 1. Crio um "irmão" novo ('newChild') pra ser a metade da direita.
    Node* newChild = new Node(child->isLeaf);
    
    // 2. Coloco esse 'newChild' na lista de filhos do 'parent',
    //    logo ao lado do 'child' original.
    parent->children.insert(
        parent->children.begin() + index + 1, newChild);
    
    // 3. "Promoção": Eu pego a chave do *meio* do 'child' lotado
    //    (posição t-1) e "subo" ela pro 'parent'. Ela vai ser
    //    o "divisor" entre o 'child' antigo e o 'newChild'.
    parent->keys.insert(parent->keys.begin() + index,
                        child->keys[t - 1]);

    // 4. Divido as chaves: O 'newChild' fica com a "metade da direita"
    //    (tudo a partir da posição 't' até o fim).
    newChild->keys.assign(child->keys.begin() + t,
                          child->keys.end());
    // 5. Corto o 'child' original pra ele ficar só com a "metade da esquerda".
    child->keys.resize(t - 1);

    // 6. Se NÃO for folha, tem que fazer a *mesma coisa* com os
    //    ponteiros de filhos (metade da direita pro 'newChild',
    //    metade da esquerda fica no 'child').
    if (!child->isLeaf) {
        newChild->children.assign(child->children.begin()
                                      + t,
                                  child->children.end());
        child->children.resize(t);
    }

    // 7. Se *FOR* folha, eu ligo a "corrente" 🔗
    //    Faço o 'newChild' apontar pra quem o 'child' apontava,
    //    e faço o 'child' apontar pro 'newChild'.
    if (child->isLeaf) {
        newChild->next = child->next;
        child->next = newChild;
    }
}

/**
 * @brief (insertNonFull) Função recursiva que desce a árvore.
 *
 * Lógica: O nome "NonFull" é porque a gente *garante* que o nó
 * que ela visita nunca tá cheio (graças ao 'splitChild' que a
 * gente chama antes).
 */
template <typename T>
void BPlusTree<T>::insertNonFull(Node* node, T key)
{
    // CASO BASE: Se cheguei na folha, é aqui que eu insiro.
    if (node->isLeaf) {
        // Uso 'upper_bound' pra achar o lugar certo e manter a
        // ordem, e 'insert' pra colocar a chave lá.
        node->keys.insert(upper_bound(node->keys.begin(),
                                      node->keys.end(),
                                      key),
                          key);
    }
    // CASO RECURSIVO: Se for um nó interno (guia), eu preciso
    // decidir pra qual filho descer.
    else {
        // Acha o caminho certo ('i+1') pra descer.
        int i = node->keys.size() - 1;
        while (i >= 0 && key < node->keys[i]) {
            i--;
        }
        i++;
        
        // A MÁGICA: *Antes* de descer, eu checo se o filho pra
        // onde eu vou ('children[i]') tá LOTADO.
        if (node->children[i]->keys.size() == 2 * t - 1) {
            // Se tiver, eu chamo o 'splitChild' AGORA.
            splitChild(node, i, node->children[i]);
            
            // Depois de dividir, pode ser que a chave que subiu
            // ('keys[i]') seja menor que a minha 'key'.
            // Se for, eu tenho que descer pro 'novo' irmão da
            // direita (i+1).
            if (key > node->keys[i]) {
                i++;
            }
        }
        // Chamo a mesma função de novo, mas um nível abaixo,
        // no filho correto (que agora *com certeza* tem espaço).
        insertNonFull(node->children[i], key);
    }
}

/**
 * @brief (remove - aux) A função recursiva e mais chata de todas.
 *
 * Lógica: É o *inverso* da inserção. A gente tem que garantir
 * que o nó pra onde a gente vai descer *nunca* esteja "pobre"
 * (com menos de 't-1' chaves). A gente arruma isso *antes* de descer.
 */
template <typename T>
void BPlusTree<T>::remove(Node* node, T key)
{
    // CASO BASE (Folha): Se tô na folha, é só achar ('find')
    // e apagar ('erase'). O balanceamento já foi feito no
    // caminho pra baixo.
    if (node->isLeaf) {
        auto it = find(node->keys.begin(), node->keys.end(),
                       key);
        if (it != node->keys.end()) {
            node->keys.erase(it);
        }
    }
    // CASO RECURSIVO (Nó Interno):
    else {
        // Acha o caminho ('idx') pra descer, ou onde a chave *deveria* estar.
        int idx = lower_bound(node->keys.begin(),
                              node->keys.end(), key)
                  - node->keys.begin();

        // CENÁRIO 1: A chave tá *aqui* nesse nó interno.
        // Eu não posso só apagar, tenho que 'substituir' ela.
        if (idx < node->keys.size()
            && node->keys[idx] == key) {
            
            // OPÇÃO A: O filho da *esquerda* ('children[idx]') é "rico" (>= t)?
            if (node->children[idx]->keys.size() >= t) {
                // Sim. Pego o 'predecessor' (maior chave da sub-árvore
                // da esquerda), subo ele pra cá e chamo 'remove'
                // pra apagar ele lá de baixo.
                Node* predNode = node->children[idx];
                while (!predNode->isLeaf) {
                    predNode = predNode->children.back();
                }
                T pred = predNode->keys.back();
                node->keys[idx] = pred;
                remove(node->children[idx], pred);
            }
            // OPÇÃO B: O filho da *direita* ('children[idx+1]') é "rico"?
            else if (node->children[idx + 1]->keys.size()
                       >= t) {
                // Sim. Pego o 'sucessor' (menor chave da sub-árvore
                // da direita). Mesma lógica.
                Node* succNode = node->children[idx + 1];
                while (!succNode->isLeaf) {
                    succNode = succNode->children.front();
                }
                T succ = succNode->keys.front();
                node->keys[idx] = succ;
                remove(node->children[idx + 1], succ);
            }
            // OPÇÃO C: Os dois filhos são "pobres".
            else {
                // Faço 'merge' (junto os dois) e chamo 'remove'
                // no nó fundido pra apagar a chave de lá.
                merge(node, idx);
                remove(node->children[idx], key);
            }
        }
        // CENÁRIO 2: A chave tá *lá embaixo*. Tenho que descer.
        else {
            // "BALANCEAMENTO PROATIVO" ⚖️
            // *Antes* de descer, checo se o filho ('children[idx]') tá "pobre"
            // (com menos de 't' chaves, ou seja, 't-1').
            if (node->children[idx]->keys.size() < t) {
                
                // Tenta OPÇÃO A: Pegar emprestado do irmão da *esquerda*
                if (idx > 0
                    && node->children[idx - 1]->keys.size()
                           >= t) {
                    borrowFromPrev(node, idx);
                }
                // Tenta OPÇÃO B: Pegar emprestado do irmão da *direita*
                else if (idx < node->children.size() - 1
                         && node->children[idx + 1]
                                ->keys.size()
                                >= t) {
                    borrowFromNext(node, idx);
                }
                // Tenta OPÇÃO C: Ninguém pode emprestar. Tem que *fundir*.
                else {
                    if (idx < node->children.size() - 1) {
                        merge(node, idx); // Funde com o da direita
                    }
                    else {
                        merge(node, idx - 1); // Funde com o da esquerda
                    }
                }
            }
            // Depois de garantir que o filho tá 'rico', eu desço.
            remove(node->children[idx], key);
        }
    }
}

/**
 * @brief (borrowFromPrev) Pega emprestado do irmão da esquerda.
 *
 * Lógica: É uma "rotação".
 * 1. O filho 'pobre' ('child') pega a chave 'separadora' do 'parent'.
 * 2. O 'parent' pega a *última* chave do irmão 'rico' ('sibling').
 * 3. O 'sibling' perde essa última chave.
 * (Se for nó interno, faz o mesmo com os ponteiros de 'children').
 */
template <typename T>
void BPlusTree<T>::borrowFromPrev(Node* node, int index)
{
    Node* child = node->children[index];
    Node* sibling = node->children[index - 1];

    child->keys.insert(child->keys.begin(),
                       node->keys[index - 1]);
    node->keys[index - 1] = sibling->keys.back();
    sibling->keys.pop_back();

    if (!child->isLeaf) {
        child->children.insert(child->children.begin(),
                               sibling->children.back());
        sibling->children.pop_back();
    }
}

/**
 * @brief (borrowFromNext) Pega emprestado do irmão da direita.
 *
 * Lógica: A mesma rotação, ao contrário.
 * 1. O 'pobre' ('child') pega a chave 'separadora' do 'parent'.
 * 2. O 'parent' pega a *primeira* chave do irmão 'rico' ('sibling').
 * 3. O 'sibling' perde essa primeira chave.
 */
template <typename T>
void BPlusTree<T>::borrowFromNext(Node* node, int index)
{
    Node* child = node->children[index];
    Node* sibling = node->children[index + 1];

    child->keys.push_back(node->keys[index]);
    node->keys[index] = sibling->keys.front();
    sibling->keys.erase(sibling->keys.begin());

    if (!child->isLeaf) {
        child->children.push_back(
            sibling->children.front());
        sibling->children.erase(sibling->children.begin());
    }
}

/**
 * @brief (merge) Funde dois nós "pobres".
 *
 * Lógica: O 'child' (da esquerda) "engole" o 'sibling' (da direita).
 * 1. O 'child' pega a chave 'separadora' do 'parent'.
 * 2. O 'child' pega *todas* as chaves do 'sibling'.
 * 3. (Se for nó interno, pega *todos* os filhos do 'sibling').
 * 4. Eu apago a chave 'separadora' e o ponteiro pro 'sibling' do 'parent'.
 * 5. Dou 'delete' no 'sibling', que agora tá vazio e não é usado.
 */
template <typename T>
void BPlusTree<T>::merge(Node* node, int index)
{
    Node* child = node->children[index];
    Node* sibling = node->children[index + 1];

    // 1. Puxa a chave separadora do pai pra dentro do 'child'
    child->keys.push_back(node->keys[index]);
    // 2. Copia todas as chaves do 'sibling' pro fim do 'child'
    child->keys.insert(child->keys.end(),
                       sibling->keys.begin(),
                       sibling->keys.end());
    // 3. Se for nó interno, copia todos os filhos também
    if (!child->isLeaf) {
        child->children.insert(child->children.end(),
                               sibling->children.begin(),
                               sibling->children.end());
    }

    // 4. Remove a chave e o ponteiro do 'parent'
    node->keys.erase(node->keys.begin() + index);
    node->children.erase(node->children.begin() + index
                         + 1);

    // 5. Libera a memória do 'sibling' que foi "engolido"
    delete sibling;
}

/**
 * @brief (printTree - aux) A função recursiva de imprimir.
 *
 * Lógica: Imprime os espaços ('level') pra indentar, depois as
 * chaves do 'node' atual, e aí chama a si mesma pra *cada*
 * filho, aumentando o 'level' em 1.
 */
template <typename T>
void BPlusTree<T>::printTree(Node* node, int level)
{
    if (node != nullptr) {
        for (int i = 0; i < level; ++i) {
            cout << "  ";
        }
        for (const T& key : node->keys) {
            cout << key << " ";
        }
        cout << endl;
        for (Node* child : node->children) {
            printTree(child, level + 1);
        }
    }
}

/**
 * @brief (printTree - public) Função "wrapper" (invólucro).
 *
 * Lógica: Só pra o usuário não ter que passar 'root' e '0'.
 */
template <typename T> void BPlusTree<T>::printTree()
{
    printTree(root, 0);
}

/**
 * @brief (search) A mais fácil.
 *
 * Lógica: É só um 'while' que desce a árvore. Em cada nó,
 * ela acha o caminho certo ('i') pra descer.
 * Se em algum momento 'key == current->keys[i]', achou (true).
 * Se chegar na folha ('isLeaf') e não achar, não existe (false).
 */
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
 * @brief (rangeQuery) A principal vantagem da B+ Tree.
 *
 * Lógica:
 * 1. Primeiro, eu desço a árvore (igual na 'search') até achar
 * a *folha* onde o 'lower' (limite inferior) estaria.
 * 2. Daí, eu entro num 'while (current != nullptr)' e saio
 * varrendo as folhas usando o ponteiro 'next' (a 'corrente' 🔗).
 * 3. Vou adicionando no 'result' todas as chaves que tão
 * entre 'lower' e 'upper'.
 * 4. Se eu achar uma chave maior que 'upper', eu paro na hora
 * (porque tá tudo ordenado).
 */
template <typename T>
vector<T> BPlusTree<T>::rangeQuery(T lower, T upper)
{
    vector<T> result;
    Node* current = root;

    // 1. Desce até a folha correta pra começar
    while (!current->isLeaf) {
        int i = 0;
        while (i < current->keys.size()
               && lower > current->keys[i]) {
            i++;
        }
        current = current->children[i];
    }

    // 2. Varre a "corrente" de folhas
    while (current != nullptr) {
        for (const T& key : current->keys) {
            // 3. Adiciona se tiver no intervalo
            if (key >= lower && key <= upper) {
                result.push_back(key);
            }
            // 4. Para se já passou do limite
            if (key > upper) {
                return result;
            }
        }
        // Pula pra próxima folha
        current = current->next;
    }
    return result;
}

/**
 * @brief (insert - public) A que o usuário chama.
 *
 * Lógica: Lida com os dois casos chatos da raiz.
 */
template <typename T> void BPlusTree<T>::insert(T key)
{
    // CASO 1: Árvore vazia. Eu só crio a 'root' como folha
    // e coloco a chave.
    if (root == nullptr) {
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

/**
 * @brief (remove - public) A que o usuário chama.
 *
 * Lógica: Lida com os casos de borda da raiz (encolhimento).
 */
template <typename T> void BPlusTree<T>::remove(T key)
{
    // Se a árvore tá vazia, não faz nada.
    if (root == nullptr) {
        return;
    }

    // Chama a 'remove' recursiva (a monstrona) pra fazer o trabalho.
    remove(root, key);

    // Caso de "encolhimento":
    // Se, depois da remoção (por causa de um 'merge'), a 'root'
    // ficar *vazia* e *não for folha* (virou uma 'casca' com 1 filho só)...
    if (root->keys.empty() && !root->isLeaf) {
        // ...eu 'promovo' esse único filho ('children[0]')
        // a ser a nova 'root' e dou 'delete' na 'casca' antiga.
        Node* tmp = root;
        root = root->children[0];
        delete tmp;
    }
}