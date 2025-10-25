/*
 * ==========================================================
 * testa_arvore_titulo.cpp - Arquivo de Teste para BPlusTreeTitulo
 * ==========================================================
 */

#include <iostream>       // Para std::cout, std::cerr
#include <string>         // Para std::string
#include <vector>         // Para std::vector (retorno do search)
#include <stdexcept>      // Para std::exception
#include <cstdio>         // Para std::remove (deletar arquivo antigo)
#include <algorithm>      // Para std::find
#include "ArvoreBMaisTitulo.hpp" // O header da sua árvore de Títulos

// --- Constantes do Teste ---
const std::string NOME_ARQUIVO_TITULO = "arvore_titulo_teste.idx";
const size_t TAMANHO_BLOCO = 4096; // 4KB

// Função auxiliar para verificar se um vetor contém um valor específico
bool contemDataPointer(const std::vector<long>& vec, long value) {
    return std::find(vec.begin(), vec.end(), value) != vec.end();
}

int main()
{
    std::cout << "--- Iniciando Teste da BPlusTreeTitulo ---" << std::endl;

    // 1. Limpa o arquivo de teste anterior, se existir
    std::remove(NOME_ARQUIVO_TITULO.c_str());
    std::cout << "Arquivo de indice de titulo antigo removido." << std::endl;

    // --- Dados de Teste ---
    const int NUM_CHAVES = 50; // Reduzido para focar nos testes de duplicata/remoção
    const ChaveTitulo TITULO_DUPLICADO("Titulo Repetido Exemplo");
    long DATA_PTR_DUP_1 = 99901;
    long DATA_PTR_DUP_2 = 99902;
    const ChaveTitulo TITULO_SIMPLES("Titulo Simples Para Teste");
    long DATA_PTR_SIMPLES = 12345;
    const ChaveTitulo TITULO_INEXISTENTE("Titulo Que Nao Existe");

    try
    {
        // --- FASE 1: Inserção ---
        {
            BPlusTreeTitulo arvore(NOME_ARQUIVO_TITULO, TAMANHO_BLOCO);
            std::cout << "\n--- FASE 1: Criando arvore e inserindo " << NUM_CHAVES << " titulos ---" << std::endl;

            for (int i = 1; i <= NUM_CHAVES; ++i)
            {
                // Gera títulos sequenciais
                std::string tituloStr = "Titulo Artigo Numero " + std::to_string(i);
                ChaveTitulo chave(tituloStr); // Converte para ChaveTitulo
                long dataPtr = i * 100;
                arvore.insert(chave, dataPtr);
            }
            std::cout << "Insercao de " << NUM_CHAVES << " titulos concluida." << std::endl;

            // Insere título simples
            std::cout << "Inserindo titulo simples: \"" << TITULO_SIMPLES.titulo << "\"" << std::endl;
            arvore.insert(TITULO_SIMPLES, DATA_PTR_SIMPLES);

            // Insere títulos duplicados
            std::cout << "Inserindo titulo duplicado: \"" << TITULO_DUPLICADO.titulo << "\" (ptr=" << DATA_PTR_DUP_1 << ")" << std::endl;
            arvore.insert(TITULO_DUPLICADO, DATA_PTR_DUP_1);
            std::cout << "Inserindo titulo duplicado: \"" << TITULO_DUPLICADO.titulo << "\" (ptr=" << DATA_PTR_DUP_2 << ")" << std::endl;
            arvore.insert(TITULO_DUPLICADO, DATA_PTR_DUP_2);


            std::cout << "\n--- FASE 2: Buscando chaves (antes de fechar) ---" << std::endl;

            // Teste de sucesso (chave única)
            ChaveTitulo chaveBuscar("Titulo Artigo Numero 10");
            std::vector<long> resultados = arvore.search(chaveBuscar);
            std::cout << "Buscando \"" << chaveBuscar.titulo << "\"... Resultados: " << resultados.size() << " (Esperado: 1)" << std::endl;
            if (resultados.size() != 1 || resultados[0] != 10 * 100) std::cerr << "!!! ERRO NO TESTE (Busca Unica)!" << std::endl;

            // Teste de sucesso (chave duplicada)
            resultados = arvore.search(TITULO_DUPLICADO);
            std::cout << "Buscando \"" << TITULO_DUPLICADO.titulo << "\"... Resultados: " << resultados.size() << " (Esperado: 2)" << std::endl;
            if (resultados.size() != 2 || !contemDataPointer(resultados, DATA_PTR_DUP_1) || !contemDataPointer(resultados, DATA_PTR_DUP_2)) {
                std::cerr << "!!! ERRO NO TESTE (Busca Duplicada)!" << std::endl;
            } else {
                 std::cout << "   Encontrados ponteiros: " << resultados[0] << ", " << resultados[1] << std::endl;
            }

            // Teste de falha (chave inexistente)
            resultados = arvore.search(TITULO_INEXISTENTE);
            std::cout << "Buscando \"" << TITULO_INEXISTENTE.titulo << "\"... Resultados: " << resultados.size() << " (Esperado: 0)" << std::endl;
            if (!resultados.empty()) std::cerr << "!!! ERRO NO TESTE (Busca Inexistente)!" << std::endl;

            std::cout << "Fechando o arquivo (saindo do escopo)..." << std::endl;
        } // 'arvore' é destruída aqui.

        // --- FASE 3: Teste de Persistência ---
        {
            std::cout << "\n--- FASE 3: Testando Persistencia (Recarregando do disco) ---" << std::endl;
            BPlusTreeTitulo arvoreDoDisco(NOME_ARQUIVO_TITULO, TAMANHO_BLOCO);
            std::cout << "Arvore recarregada do arquivo '" << NOME_ARQUIVO_TITULO << "'." << std::endl;

            // Testar chave única
            ChaveTitulo chaveBuscar("Titulo Artigo Numero 10");
            std::vector<long> resultados = arvoreDoDisco.search(chaveBuscar);
            std::cout << "Buscando \"" << chaveBuscar.titulo << "\"... Resultados: " << resultados.size() << " (Esperado: 1)" << std::endl;
            if (resultados.size() != 1 || resultados[0] != 10 * 100) std::cerr << "!!! ERRO NO TESTE DE PERSISTENCIA (Unica)!" << std::endl;

            // Testar chave duplicada novamente
             resultados = arvoreDoDisco.search(TITULO_DUPLICADO);
            std::cout << "Buscando \"" << TITULO_DUPLICADO.titulo << "\"... Resultados: " << resultados.size() << " (Esperado: 2)" << std::endl;
            if (resultados.size() != 2 || !contemDataPointer(resultados, DATA_PTR_DUP_1) || !contemDataPointer(resultados, DATA_PTR_DUP_2)) {
                std::cerr << "!!! ERRO NO TESTE DE PERSISTENCIA (Duplicada)!" << std::endl;
            }

            // Testar chave inexistente
            resultados = arvoreDoDisco.search(TITULO_INEXISTENTE);
            std::cout << "Buscando \"" << TITULO_INEXISTENTE.titulo << "\"... Resultados: " << resultados.size() << " (Esperado: 0)" << std::endl;
            if (!resultados.empty()) std::cerr << "!!! ERRO NO TESTE DE PERSISTENCIA (Inexistente)!" << std::endl;

            // ==================================================
            // --- FASE 4: Testando Remocao ---
            // ==================================================
            std::cout << "\n--- FASE 4: Testando Remocao ---" << std::endl;
            
            // --- Teste 1: Remoção Simples ---
            ChaveTitulo chaveRemoverSimples("Titulo Artigo Numero 30");
            long ptrRemoverSimples = 30 * 100;
            std::cout << "Removendo \"" << chaveRemoverSimples.titulo << "\" (ptr=" << ptrRemoverSimples << ")..." << std::endl;
            arvoreDoDisco.remove(chaveRemoverSimples, ptrRemoverSimples);
            resultados = arvoreDoDisco.search(chaveRemoverSimples);
            std::cout << "Buscando removido \"" << chaveRemoverSimples.titulo << "\"... Resultados: " << resultados.size() << " (Esperado: 0)" << std::endl;
            if (!resultados.empty()) std::cerr << "!!! ERRO NO TESTE DE REMOCAO (1)!" << std::endl;

            // --- Teste 2: Remoção de Chave Inexistente ---
            std::cout << "Removendo inexistente \"" << TITULO_INEXISTENTE.titulo << "\"..." << std::endl;
            arvoreDoDisco.remove(TITULO_INEXISTENTE, 0); // O dataPointer não importa aqui
            resultados = arvoreDoDisco.search(TITULO_INEXISTENTE);
            std::cout << "Buscando \"" << TITULO_INEXISTENTE.titulo << "\"... Resultados: " << resultados.size() << " (Esperado: 0)" << std::endl;
            if (!resultados.empty()) std::cerr << "!!! ERRO NO TESTE DE REMOCAO (2)!" << std::endl;

            // --- Teste 3: Remoção de Chaves Duplicadas (Específica) ---
            std::cout << "Removendo primeira copia de \"" << TITULO_DUPLICADO.titulo << "\" (ptr=" << DATA_PTR_DUP_1 << ")..." << std::endl;
            arvoreDoDisco.remove(TITULO_DUPLICADO, DATA_PTR_DUP_1); 
            resultados = arvoreDoDisco.search(TITULO_DUPLICADO);
            std::cout << "Buscando \"" << TITULO_DUPLICADO.titulo << "\" de novo... Resultados: " << resultados.size() << " (Esperado: 1)" << std::endl;
            if (resultados.size() != 1 || !contemDataPointer(resultados, DATA_PTR_DUP_2)) {
                 std::cerr << "!!! ERRO NO TESTE DE REMOCAO (3)! Esperado [" << DATA_PTR_DUP_2 << "]" << std::endl;
            } else {
                 std::cout << "   Ponteiro restante: " << resultados[0] << std::endl;
            }


            std::cout << "Removendo segunda copia de \"" << TITULO_DUPLICADO.titulo << "\" (ptr=" << DATA_PTR_DUP_2 << ")..." << std::endl;
            arvoreDoDisco.remove(TITULO_DUPLICADO, DATA_PTR_DUP_2); 
            resultados = arvoreDoDisco.search(TITULO_DUPLICADO);
            std::cout << "Buscando \"" << TITULO_DUPLICADO.titulo << "\" final... Resultados: " << resultados.size() << " (Esperado: 0)" << std::endl;
            if (!resultados.empty()) std::cerr << "!!! ERRO NO TESTE DE REMOCAO (4)!" << std::endl;

            // --- Teste 4: Verificar se outras chaves ainda existem ---
            ChaveTitulo chaveVerificar("Titulo Artigo Numero 5");
            long ptrEsperado = 5 * 100;
            std::cout << "Verificando \"" << chaveVerificar.titulo << "\" (ainda existe)..." << std::endl;
            resultados = arvoreDoDisco.search(chaveVerificar);
             if (resultados.size() != 1 || resultados[0] != ptrEsperado) std::cerr << "!!! ERRO, CHAVE \"" << chaveVerificar.titulo << "\" SUMIU INDEVIDAMENTE!" << std::endl;

        } // Fim do escopo da 'arvoreDoDisco'

        // ==================================================
        // --- FASE 5: Persistência Pós-Remoção ---
        // ==================================================
        {
            std::cout << "\n--- FASE 5: Testando Persistencia (Apos Remocao) ---" << std::endl;
            BPlusTreeTitulo arvoreAposRemocao(NOME_ARQUIVO_TITULO, TAMANHO_BLOCO);
            std::cout << "Arvore recarregada apos remocoes." << std::endl;

            // Chave que deve ter sumido
            ChaveTitulo chaveRemovida("Titulo Artigo Numero 30");
            std::vector<long> resultados = arvoreAposRemocao.search(chaveRemovida);
            std::cout << "Buscando removido \"" << chaveRemovida.titulo << "\"... Resultados: " << resultados.size() << " (Esperado: 0)" << std::endl;
            if (!resultados.empty()) std::cerr << "!!! ERRO NO TESTE DE PERSISTENCIA (1)!" << std::endl;

            // Chave duplicada que deve ter sumido
            resultados = arvoreAposRemocao.search(TITULO_DUPLICADO);
            std::cout << "Buscando removido \"" << TITULO_DUPLICADO.titulo << "\"... Resultados: " << resultados.size() << " (Esperado: 0)" << std::endl;
            if (!resultados.empty()) std::cerr << "!!! ERRO NO TESTE DE PERSISTENCIA (2)!" << std::endl;

            // Chave que deve existir
            ChaveTitulo chaveExistente("Titulo Artigo Numero 5");
            long ptrEsperado = 5 * 100;
            resultados = arvoreAposRemocao.search(chaveExistente);
            std::cout << "Buscando existente \"" << chaveExistente.titulo << "\"... Resultados: " << resultados.size() << " (Esperado: 1)" << std::endl;
            if (resultados.size() != 1 || resultados[0] != ptrEsperado) std::cerr << "!!! ERRO NO TESTE DE PERSISTENCIA (3)!" << std::endl;
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