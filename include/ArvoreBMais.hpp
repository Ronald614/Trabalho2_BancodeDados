#ifndef ARVOREBMAIS_HPP
#define ARVOREBMAIS_HPP

#include <string>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <math.h>
#include <cmath> 
#include "GerenciadorIndice.hpp"

// --- Structs Comuns ---

#pragma pack(push, 1)

struct CabecalhoIndice {

    long idRaiz;
    int numBlocos;
    long tamanhoBloco;

};

#pragma pack(pop) 

#pragma pack(push, 1)

struct ChaveTitulo {

    char titulo[300]; 

    bool operator<(const ChaveTitulo& other) const { return strncmp(titulo, other.titulo, 300) < 0; }
    bool operator>(const ChaveTitulo& other) const { return strncmp(titulo, other.titulo, 300) > 0; }
    bool operator==(const ChaveTitulo& other) const { return strncmp(titulo, other.titulo, 300) == 0; }
    bool operator>=(const ChaveTitulo& other) const { return strncmp(titulo, other.titulo, 300) >= 0; }

    ChaveTitulo() { memset(titulo, 0, 300); } 
    ChaveTitulo(const char* str) { strncpy(titulo, str, 299); titulo[299] = '\0'; }
    ChaveTitulo(const std::string& str) : ChaveTitulo(str.c_str()) {}

};

#pragma pack(pop)

// --- Nó com Template ---
template <typename KeyType>

struct No {

    bool ehFolha;
    int numChaves;
    long proximo; 
    long selfId; 
    std::vector<KeyType> vetorChaves;
    std::vector<long> vetorApontadores;
    
    No(bool ehFolha = false) : ehFolha(ehFolha), numChaves(0), proximo(-1), selfId(-1) {}

};

// --- Classe BPlusTree com Template ---
template <typename KeyType>
class BPlusTree {
private:
    // --- Atributos ---
    int tamanhoBloco;
    std::string nomeArquivo;
    int m; // Ordem da árvore
    GerenciadorIndice gerenciador;
    long idRaiz;
    long totalBlocos;

    // --- Métodos Privados de I/O e Nó ---

    // Escreve o cabeçalho no disco
    void escreverCabecalho() {
        CabecalhoIndice hdr;
        hdr.idRaiz = this->idRaiz;
        hdr.tamanhoBloco = this->tamanhoBloco;
        hdr.numBlocos = this->totalBlocos;

        char *buffer = new char[tamanhoBloco];
        memset(buffer, 0, tamanhoBloco);
        memcpy(buffer, &hdr, sizeof(CabecalhoIndice));
        gerenciador.escreveBloco(0, buffer);
        delete[] buffer;
    }

    // Lê o cabeçalho do disco
    void lerCabecalho() {
        char *buffer = new char[tamanhoBloco];
        gerenciador.lerBloco(0, buffer);
        CabecalhoIndice hdr;
        memcpy(&hdr, buffer, sizeof(CabecalhoIndice));
        delete[] buffer;

        this->idRaiz = hdr.idRaiz;
        this->tamanhoBloco = hdr.tamanhoBloco;
        this->totalBlocos = hdr.numBlocos;
    }

    // Serializa um nó
    void serializaNo(const No<KeyType> &no, char *buffer) {
        char *ptr = buffer;
        memcpy(ptr, &no.ehFolha, sizeof(bool));
        ptr += sizeof(bool);
        memcpy(ptr, &no.numChaves, sizeof(int));
        ptr += sizeof(int);
        memcpy(ptr, &no.proximo, sizeof(long));
        ptr += sizeof(long);

        memcpy(ptr, no.vetorChaves.data(), no.numChaves * sizeof(KeyType));
        ptr += no.numChaves * sizeof(KeyType); 

        if (no.ehFolha) {
            memcpy(ptr, no.vetorApontadores.data(), no.numChaves * sizeof(long));
        } else {
            memcpy(ptr, no.vetorApontadores.data(), (no.numChaves + 1) * sizeof(long));
        }
    }
    
    // Deserializa um nó
    void deserializaNo(const char *buffer, No<KeyType> &no) {
        const char *ptr = buffer;
        memcpy(&no.ehFolha, ptr, sizeof(bool));
        ptr += sizeof(bool);
        memcpy(&no.numChaves, ptr, sizeof(int));
        ptr += sizeof(int);
        memcpy(&no.proximo, ptr, sizeof(long));
        ptr += sizeof(long);

        no.vetorChaves.resize(no.numChaves);
        memcpy(no.vetorChaves.data(), ptr, no.numChaves * sizeof(KeyType));
        ptr += no.numChaves * sizeof(KeyType);

        if (no.ehFolha) {
            no.vetorApontadores.resize(no.numChaves);
            memcpy(no.vetorApontadores.data(), ptr, no.numChaves * sizeof(long));
        } else {
            no.vetorApontadores.resize(no.numChaves + 1);
            memcpy(no.vetorApontadores.data(), ptr, (no.numChaves + 1) * sizeof(long));
        }
    }

    // Escreve um nó no disco
    void escreverNo(No<KeyType> *no) {
        char *buffer = new char[tamanhoBloco];
        serializaNo(*no, buffer);
        gerenciador.escreveBloco(no->selfId, buffer);
        delete[] buffer;
    }

    // Lê um nó do disco
    No<KeyType> *lerNo(long blockNum, No<KeyType> *no) {
        char *buffer = new char[tamanhoBloco];
        gerenciador.lerBloco(blockNum, buffer);
        deserializaNo(buffer, *no);
        no->selfId = blockNum;
        delete[] buffer;
        return no;
    }

    // Retorna um novo ID para um nó
    long getNovoId() {
        long id = this->totalBlocos;
        this->totalBlocos++;
        return id;
    }

    // --- Métodos Privados de Inserção ---

    void splitChild(No<KeyType> *parent, int irmaoIndex) {
        No<KeyType> *novoIrmao = new No<KeyType>();
        novoIrmao->selfId = this->getNovoId();

        long idFilho = parent->vetorApontadores[irmaoIndex];
        No<KeyType> *irmao = new No<KeyType>();
        lerNo(idFilho, irmao);

        novoIrmao->ehFolha = irmao->ehFolha;

        int indiceMeio;
        KeyType chavePromovida;

        if (irmao->ehFolha) {
            indiceMeio = m / 2;
            chavePromovida = irmao->vetorChaves[indiceMeio];

            novoIrmao->vetorChaves.assign(irmao->vetorChaves.begin() + indiceMeio, irmao->vetorChaves.end());
            novoIrmao->vetorApontadores.assign(irmao->vetorApontadores.begin() + indiceMeio, irmao->vetorApontadores.end());

            novoIrmao->numChaves = m - indiceMeio;
            irmao->numChaves = indiceMeio;

            irmao->vetorChaves.resize(indiceMeio);
            irmao->vetorApontadores.resize(indiceMeio);

            novoIrmao->proximo = irmao->proximo;
            irmao->proximo = novoIrmao->selfId;
        } else {
            indiceMeio = (m - 1) / 2;
            chavePromovida = irmao->vetorChaves[indiceMeio];

            novoIrmao->vetorChaves.assign(irmao->vetorChaves.begin() + indiceMeio + 1, irmao->vetorChaves.end());
            novoIrmao->vetorApontadores.assign(irmao->vetorApontadores.begin() + indiceMeio + 1, irmao->vetorApontadores.end());

            novoIrmao->numChaves = (m - 1) - (indiceMeio + 1);
            irmao->numChaves = indiceMeio;

            irmao->vetorChaves.resize(indiceMeio);
            irmao->vetorApontadores.resize(indiceMeio + 1);
        }

        parent->vetorChaves.insert(parent->vetorChaves.begin() + irmaoIndex, chavePromovida);
        parent->vetorApontadores.insert(parent->vetorApontadores.begin() + irmaoIndex + 1, novoIrmao->selfId);
        parent->numChaves++;

        escreverNo(parent);
        escreverNo(irmao);
        escreverNo(novoIrmao);

        delete irmao;
        delete novoIrmao;
    }

    void insertNonFull(No<KeyType> *noAtual, const KeyType& key, long dataPointer) {
        if (noAtual->ehFolha) {
            int i = 0;
            while (i < noAtual->numChaves && key >= noAtual->vetorChaves[i]) {
                i++;
            }
            noAtual->vetorChaves.insert(noAtual->vetorChaves.begin() + i, key);
            noAtual->vetorApontadores.insert(noAtual->vetorApontadores.begin() + i, dataPointer);
            noAtual->numChaves++;
            escreverNo(noAtual);
            return;
        } else {
            int i = 0;
            while (i < noAtual->numChaves && key >= noAtual->vetorChaves[i]) {
                i++;
            }
            
            long idFilho = noAtual->vetorApontadores[i];
            No<KeyType> *irmaoNo = new No<KeyType>();
            lerNo(idFilho, irmaoNo);

            bool irmaoIsFull = false;
            if (irmaoNo->ehFolha) {
                if (irmaoNo->numChaves == m) irmaoIsFull = true;
            } else {
                if (irmaoNo->numChaves == m - 1) irmaoIsFull = true;
            }

            if (irmaoIsFull) {
                splitChild(noAtual, i);
                if (key > noAtual->vetorChaves[i]) {
                    i++;
                }
                idFilho = noAtual->vetorApontadores[i];
                lerNo(idFilho, irmaoNo);
            }
            insertNonFull(irmaoNo, key, dataPointer);
            delete irmaoNo;
        }
    }

    // --- Método Privado de Busca ---

    std::vector<long> search_internal(const KeyType& key) {
        std::vector<long> resultados;
        if (idRaiz == -1) {
            return resultados; 
        }

        No<KeyType> *noAtual = new No<KeyType>();
        lerNo(idRaiz, noAtual);

        // 1. Desce até a folha
        while (!noAtual->ehFolha) {
            int i = 0;
            while (i < noAtual->numChaves && key >= noAtual->vetorChaves[i]) {
                 if (key == noAtual->vetorChaves[i]) {
                     i++; 
                     break; 
                 }
                 if (key > noAtual->vetorChaves[i]) {
                      i++;
                 } else {
                     break; 
                 }
            }
            long filhoId = noAtual->vetorApontadores[i]; 
            lerNo(filhoId, noAtual); 
        }

        // 2. Varre a(s) folha(s)
        int i = 0;
        while (i < noAtual->numChaves && key > noAtual->vetorChaves[i]) {
            i++;
        }

        bool continuarVarrendo = true;
        while (continuarVarrendo) {
            while (i < noAtual->numChaves) {
                if (noAtual->vetorChaves[i] == key) {
                    resultados.push_back(noAtual->vetorApontadores[i]);
                    i++; 
                } else {
                    continuarVarrendo = false; 
                    break; 
                }
            }

            if (!continuarVarrendo) break;

            if (noAtual->proximo != -1) {
                lerNo(noAtual->proximo, noAtual);
                i = 0; 
            } else {
                continuarVarrendo = false;
            }
        }
        delete noAtual;
        return resultados; 
    }

public:
    // --- Construtor Público ---
    BPlusTree(const std::string &nomeArquivo, const size_t tamanhoBloco_arg)
        : tamanhoBloco(static_cast<int>(tamanhoBloco_arg)), 
          nomeArquivo(nomeArquivo),
          gerenciador(nomeArquivo, tamanhoBloco_arg),
          idRaiz(-1),
          totalBlocos(0) {
        try {
            if (gerenciador.getTamanhoArquivo() > 0) {
                lerCabecalho();
                if (static_cast<int>(tamanhoBloco_arg) != this->tamanhoBloco) {
                    throw std::runtime_error("Erro: O tamanho do bloco fornecido é inconsistente com o do arquivo!");
                }
            } else {
                this->idRaiz = -1;
                this->totalBlocos = 1;
                escreverCabecalho();
            }

            // --- CÁLCULO DE 'm' (ORDEM) ---
            int tamMetadados = sizeof(bool) + sizeof(int) + sizeof(long);
            m = floor((this->tamanhoBloco - tamMetadados) / (sizeof(KeyType) + sizeof(long)));
        } catch (const std::exception &e) {
            throw;
        }
    }

    // --- Destrutor ---
    ~BPlusTree() {}

    // --- Métodos Públicos de Interface ---

    // Inserção
    void insert(const KeyType& key, long dataPointer) {
        if (idRaiz == -1) {
            No<KeyType> *primeiraRaiz = new No<KeyType>(true);
            primeiraRaiz->selfId = this->getNovoId();
            this->idRaiz = primeiraRaiz->selfId;
            primeiraRaiz->vetorChaves.push_back(key);
            primeiraRaiz->vetorApontadores.push_back(dataPointer);
            primeiraRaiz->numChaves = 1;
            escreverNo(primeiraRaiz);
            escreverCabecalho();
            delete primeiraRaiz; 
            return;
        } else {
            No<KeyType> *raizInicial = new No<KeyType>();
            lerNo(this->idRaiz, raizInicial);

            bool raizCheia = false;
            if (raizInicial->ehFolha) {
                if (raizInicial->numChaves == m) raizCheia = true;
            } else {
                if (raizInicial->numChaves == m - 1) raizCheia = true;
            }

            if (raizCheia) {
                No<KeyType> *novaRaiz = new No<KeyType>(false);
                novaRaiz->selfId = this->getNovoId();
                novaRaiz->vetorApontadores.push_back(this->idRaiz);
                this->idRaiz = novaRaiz->selfId;
                escreverCabecalho();
                splitChild(novaRaiz, 0);
                insertNonFull(novaRaiz, key, dataPointer);
                delete novaRaiz;
            } else {
                insertNonFull(raizInicial, key, dataPointer);
            }
            delete raizInicial;
        }
    }

    // Busca
    std::vector<long> search(const KeyType& key) {
        return search_internal(key);
    }

    // Getters de estatísticas
    long getIndexBlocosLidos() const { return gerenciador.getBlocosLidos(); }
    long getIndexBlocosEscritos() const { return gerenciador.getBlocosEscritos(); }
    long getIndexTotalBlocos() const { return totalBlocos; }

    void flush() {
        gerenciador.flush();
    }
};

#endif