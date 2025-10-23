#include "BlockManager.hpp"
#include "OSInfo.hpp"

int main(int argc, char* argv[]){
    
    if(argc != 2){
    
        std::cout << "Falta o arquivo de entrada.\n";
        return 0;
    
    }
    
    char* path = argv[1];
    
    FILE* arquivo;
    
    fopen(path, "r");

    int Block_Size = obter_tamanho_bloco_fs(path);

    BlockManager gerenciador = BlockManager(path, Block_Size);

    long primeiro = gerenciador.allocateBlock();
    std::cout << "primeiro:" << primeiro << "\n";

    long segundo = gerenciador.allocateBlock();
    std::cout << "segundo:" << segundo << "\n";

}