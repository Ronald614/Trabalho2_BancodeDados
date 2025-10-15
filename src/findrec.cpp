#include <iostream>

int main(int argc, char* argv[]) {

    std::cout << "--- Programa FINDREC ---" << std::endl;
    
    if (argc > 1) {
    
        std::cout << "Buscando pelo ID: " << argv[1] << std::endl;
    
    } 
    
    else {
    
        std::cout << "Erro: Nenhum ID especificado." << std::endl;
    
    }
    
    std::cout << "Ambiente configurado com sucesso!" << std::endl;
    
    return 0;

}