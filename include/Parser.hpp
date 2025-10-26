#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>
#include <iostream>

#include "Artigo.hpp"

/**
 * @brief Imprime todos os campos de um struct Artigo para o console.
 * @param artigo O Artigo a ser impresso.
 */
void printArtigo(const Artigo& artigo);

/**
 * @brief Copia uma string de origem para um array de char de destino de forma segura.
 * Garante que a string de destino seja terminada em nulo e evita overflow.
 * @param destino Ponteiro para o array de char de destino.
 * @param origem A string std::string de origem.
 * @param tamanhoDestino O tamanho total do array de destino.
 */
void copiarStringSeguro(char* destino, const std::string& origem, size_t tamanhoDestino);

/**
 * @brief Converte uma string para inteiro, retornando 0 em caso de string vazia ou erro de conversão.
 * @param texto A string a ser convertida.
 * @return O valor inteiro ou 0 em caso de falha.
 */
int stringParaIntSeguro(const std::string& texto);

/**
 * @brief Limpa e sanitiza um campo lido de um CSV.
 * Remove as aspas delimitadoras (se existirem) e substitui "NULL" e strings vazias por "".
 * @param campo_bruto A string do campo CSV, potencialmente entre aspas.
 * @return A string do campo limpo.
 */
std::string limpaCampo(const std::string& campo_bruto);

/**
 * @brief Divide uma linha de CSV, tratando corretamente delimitadores dentro de aspas duplas.
 * Implementa um limite para os 6 primeiros delimitadores (;) para garantir que o resto seja o snippet.
 * @param linha A linha completa do CSV.
 * @return Um vetor de strings contendo os campos.
 */
std::vector<std::string> divideCSVLinha(const std::string& linha);

/**
 * @brief Analisa (parseia) uma linha de CSV e preenche um struct Artigo.
 * Usa as funções auxiliares de limpeza e conversão.
 * @param linha A linha de CSV a ser processada.
 * @param artigo_saida O struct Artigo a ser preenchido com os dados.
 * @return 'true' se o parsing foi bem-sucedido e o Artigo_saida foi preenchido; 'false' caso contrário.
 */
bool parseCSVLinha(const std::string& linha, Artigo& artigo_saida);

#endif // PARSER_HPP