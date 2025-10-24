/*
 * ==========================================================
 * main.cpp - Arquivo de Teste para BPlusTreeInt
 * ==========================================================
 */

#include <iostream>       // Para std::cout, std::cerr
#include <string>         // Para std::string
#include <stdexcept>      // Para std::exception
#include <cstdio>         // Para std::remove (deletar arquivo antigo)
#include "Arvorebmais.hpp" // O header da sua árvore

// --- Constantes do Teste ---
const std::string NOME_ARQUIVO = "arvore_teste.idx";
const size_t TAMANHO_BLOCO = 4096; // 4KB, como no nosso exemplo

int main()
{
    std::cout << "--- Iniciando Teste da BPlusTreeInt ---" << std::endl;

    // 1. Limpa o arquivo de teste anterior, se existir
    std::remove(NOME_ARQUIVO.c_str());
    std::cout << "Arquivo de indice antigo removido." << std::endl;

    try
    {
        // --- FASE 1: Inserção ---
        // Usamos um bloco de escopo {} para que a 'arvore' seja destruída
        // ao final do bloco, forçando o fechamento do arquivo.
        {
            BPlusTreeInt arvore(NOME_ARQUIVO, TAMANHO_BLOCO);
            std::cout << "\n--- FASE 1: Criando arvore e inserindo 500 chaves sequenciais ---" << std::endl;

            // Com m ~= 339, 500 chaves forçarão pelo menos um split de folha.
            const int NUM_CHAVES = 500;
            for (int i = 1; i <= NUM_CHAVES; ++i)
            {
                // Vamos usar um dado de teste previsível: chave * 100
                long dataPtr = i * 100;
                arvore.insert(i, dataPtr);
            }
            std::cout << "Insercao de " << NUM_CHAVES << " chaves concluida." << std::endl;

            // Insere uma chave não-sequencial para testar a descida e split
            int chaveExtra = 25;
            long dadoExtra = 2525;
            std::cout << "Inserindo chave extra " << chaveExtra << " (com dado " << dadoExtra << ")" << std::endl;
            arvore.insert(chaveExtra, dadoExtra);


            std::cout << "\n--- FASE 2: Buscando chaves (antes de fechar) ---" << std::endl;

            // Teste de sucesso (chave original)
            int chaveBuscar = 50;
            long dado = arvore.search(chaveBuscar);
            std::cout << "Buscando chave " << chaveBuscar << "... Resultado: " << dado << " (Esperado: " << 50 * 100 << ")" << std::endl;
            if (dado != 50 * 100) std::cerr << "!!! ERRO NO TESTE!" << std::endl;

            // Teste de sucesso (chave duplicada)
            // Sua lógica de busca (lower_bound) e inserção (upper_bound)
            // deve encontrar a *primeira* chave inserida.
            chaveBuscar = 25;
            dado = arvore.search(chaveBuscar);
            std::cout << "Buscando chave " << chaveBuscar << "... Resultado: " << dado << " (Esperado: " << 25 * 100 << ")" << std::endl;
            if (dado != 25 * 100) std::cerr << "!!! ERRO NO TESTE!" << std::endl;


            // Teste de falha (chave inexistente)
            chaveBuscar = 999;
            dado = arvore.search(chaveBuscar);
            std::cout << "Buscando chave " << chaveBuscar << "... Resultado: " << dado << " (Esperado: -1)" << std::endl;
            if (dado != -1) std::cerr << "!!! ERRO NO TESTE!" << std::endl;

            std::cout << "Fechando o arquivo (saindo do escopo)..." << std::endl;
        } // 'arvore' é destruída aqui.

        // --- FASE 3: Teste de Persistência ---
        // Agora, criamos um NOVO objeto de árvore. O construtor
        // deve carregar o cabeçalho e os dados do arquivo.
        {
            std::cout << "\n--- FASE 3: Testando Persistencia (Recarregando do disco) ---" << std::endl;
            BPlusTreeInt arvoreDoDisco(NOME_ARQUIVO, TAMANHO_BLOCO);
            std::cout << "Arvore recarregada do arquivo '" << NOME_ARQUIVO << "'." << std::endl;

            // Testar uma chave do fim (que estava em uma folha dividida)
            int chaveBuscar = 499;
            long dado = arvoreDoDisco.search(chaveBuscar);
            std::cout << "Buscando chave " << chaveBuscar << "... Resultado: " << dado << " (Esperado: " << 499 * 100 << ")" << std::endl;
            if (dado != 499 * 100) std::cerr << "!!! ERRO NO TESTE DE PERSISTENCIA!" << std::endl;

            // Testar a chave duplicada novamente
            chaveBuscar = 25;
            dado = arvoreDoDisco.search(chaveBuscar);
            std::cout << "Buscando chave " << chaveBuscar << "... Resultado: " << dado << " (Esperado: " << 25 * 100 << ")" << std::endl;
            if (dado != 25 * 100) std::cerr << "!!! ERRO NO TESTE DE PERSISTENCIA!" << std::endl;

            // Testar chave inexistente
            chaveBuscar = -5;
            dado = arvoreDoDisco.search(chaveBuscar);
            std::cout << "Buscando chave " << chaveBuscar << "... Resultado: " << dado << " (Esperado: -1)" << std::endl;
            if (dado != -1) std::cerr << "!!! ERRO NO TESTE DE PERSISTENCIA!" << std::endl;
        }

        std::cout << "\n--- Teste Concluido com Sucesso ---" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "ERRO FATAL: O teste falhou com uma excecao: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}