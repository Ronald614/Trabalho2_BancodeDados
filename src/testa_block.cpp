#include "GerenciaBlocos.hpp"
#include "OSInfo.hpp"

int main(int argc, char* argv[]){
    
    if(argc != 2){
    
        std::cout << "Falta o arquivo de entrada.\n";
        return 0;
    
    }
    
    char* path = argv[1];
    
    FILE* arquivo;
    
    fopen(path, "r");

    int Block_Size = obter_tamanho_bloco_fs(".");

    GerenciaBlocos gerenciador = GerenciaBlocos(path, Block_Size);

    long primeiro = gerenciador.retornaNovoId();
    std::cout << "primeiro:" << primeiro << "\n";

    char* buffer = new char[4095];
    gerenciador.escreveBloco(primeiro, buffer);

    long segundo = gerenciador.retornaNovoId();
    std::cout << "segundo:" << segundo << "\n";

}