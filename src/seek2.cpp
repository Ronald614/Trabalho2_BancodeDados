#include <iostream>

int main(int argc, char* argv[]) {

    std::cout << "--- Programa SEEK2 ---" << std::endl;
    
    if (argc > 1) {
    
        std::cout << "Buscando (índice secundário) pelo Título: \"" << argv[1] << "\"" << std::endl;
    
    }
    
    else {
    
        std::cout << "Erro: Nenhum Título especificado." << std::endl;
    
    }
    
    std::cout << "Ambiente configurado com sucesso!" << std::endl;
    
    return 0;

}