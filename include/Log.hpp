#ifndef LOG_HPP
#define LOG_HPP

#include <string>

/**
 * @enum LogLevel
 * @brief Define os níveis de log em ordem de verbosidade.
 */

enum class LogLevel {
    
    ERROR = 0, // Apenas erros fatais
    WARN  = 1, // Avisos e erros
    INFO  = 2, // Informações padrão (default)
    DEBUG = 3  // Informações de debug (mais verboso)

};

/**
 * @brief Lê a variável de ambiente LOG_LEVEL e configura o nível global.
 * Deve ser chamada uma vez no início do main() de cada programa.
 */
void log_init();

/**
 * @brief Imprime uma mensagem de ERRO (sempre visível) em std::cerr.
 */
void log_error(const std::string& msg);

/**
 * @brief Imprime uma mensagem de AVISO em std::cerr, se o nível for WARN ou superior.
 */
void log_warn(const std::string& msg);

/**
 * @brief Imprime uma mensagem de INFORMAÇÃO em std::cout, se o nível for INFO ou superior.
 */
void log_info(const std::string& msg);

/**
 * @brief Imprime uma mensagem de DEBUG em std::cout, se o nível for DEBUG.
 */
void log_debug(const std::string& msg);

#endif // LOG_HPP