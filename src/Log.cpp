#include "Log.hpp"
#include <iostream>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <cctype>

static LogLevel g_currentLevel = LogLevel::INFO;

void log_init() {

    // Lê a variável de ambiente "LOG_LEVEL"
    const char* levelStr = std::getenv("LOG_LEVEL");

    if (levelStr == nullptr) {
    
        g_currentLevel = LogLevel::INFO;
    
        return;
    
    }

    std::string level(levelStr);
    
    // Converte para minúsculo para ser case-insensitive
    std::transform(level.begin(), level.end(), level.begin(),[](unsigned char c){ return std::tolower(c); });

    if (level == "debug") {

        g_currentLevel = LogLevel::DEBUG;

    }
    
    else if (level == "info") {

        g_currentLevel = LogLevel::INFO;

    }
    
    else if (level == "warn") {
        
        g_currentLevel = LogLevel::WARN;
    }
    
    else if (level == "error") {
        
        g_currentLevel = LogLevel::ERROR;
    }
    
    else {
        
        g_currentLevel = LogLevel::INFO;
    
    }

}

void log_error(const std::string& msg) {

    if (g_currentLevel >= LogLevel::ERROR) {

        std::cerr << "[ERROR] " << msg << std::endl;

    }

}

void log_warn(const std::string& msg) {

    // Avisos (nível 1)
    if (g_currentLevel >= LogLevel::WARN) {

        std::cerr << "[WARN] " << msg << std::endl;

    }

}

void log_info(const std::string& msg) {

    // Informações (nível 2)
    if (g_currentLevel >= LogLevel::INFO) {

        std::cout << "[INFO] " << msg << std::endl;

    }

}

void log_debug(const std::string& msg) {
    
    // Debug (nível 3)
    if (g_currentLevel >= LogLevel::DEBUG) {
    
        std::cout << "[DEBUG] " << msg << std::endl;
    
    }

}