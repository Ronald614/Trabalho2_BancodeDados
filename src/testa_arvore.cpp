#include "Arvorebmais.hpp"
#include <iostream>


int main(int argc, char* argv[]){
    if(argc < 2){
        std::cin << "Erro o numero de argumentos esta errado" << std::end;
    }
    std::string nome_arquivo = argv[1];
    BPlusTreeInt arvore(nome_arquivo, 4096);

    int vetor_chaves[] = {10, 20, 5, 6, 12, 30, 7, 17, 3, 25, 15, 27, 8, 1, 4, 9, 11, 13, 14, 16,   
                         18, 19, 21, 22, 23, 24, 26, 28, 29, 31};
    for(int chave : vetor_chaves){
        arvore.insert(chave, chave * 100); // exemplo de ponteiro de dado
    }

    for(int chave : vetor_chaves){
        long resultado = arvore.search(chave);
        if(resultado != -1){
            std::cout << "Chave " << chave << " encontrada com ponteiro de dado: " << resultado << std::endl;
        } else {
            std::cout << "Chave " << chave << " nao encontrada." << std::endl;
        }
    }
}