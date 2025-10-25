#include "gerenciador_de_blocos.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <cerrno>
#include <filesystem>

// Includes para mmap e gerenciamento de arquivos.
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

GerenciadorDeBlocos::GerenciadorDeBlocos(const std::string& caminho, size_t tamanho)
    : caminho_arquivo(caminho),
      arquivo_fd(-1),
      tamanho_bloco(tamanho),
      mapa_memoria(nullptr),
      tamanho_total_arquivo(0),
      blocos_lidos(0),
      blocos_escritos(0) {

    if (tamanho_bloco == 0) {
    
        throw std::invalid_argument("[Gerenciador de Blocos] O tamanho do bloco nao pode ser zero.");
    
    }

    try {
        
        std::filesystem::path p(caminho_arquivo);
        
        std::filesystem::path diretorio = p.parent_path();

        if (!diretorio.empty()) {

            std::filesystem::create_directories(diretorio);

        }

    }
    
    catch (const std::filesystem::filesystem_error& e) {
        
        throw std::runtime_error("[Gerenciador de Blocos] Erro ao criar diretorio: " + std::string(e.what()));
    
    }

    arquivo_fd = open(caminho_arquivo.c_str(), O_RDWR | O_CREAT, (mode_t)0600);
    
    if (arquivo_fd == -1) {
    
        throw std::runtime_error("[Gerenciador de Blocos] Erro ao abrir/criar arquivo: " + std::string(std::strerror(errno)));
    
    }

    struct stat info_stat;
    
    if (fstat(arquivo_fd, &info_stat) == -1) {
    
        close(arquivo_fd);
    
        throw std::runtime_error("[Gerenciador de Blocos] Erro ao obter stat do arquivo: " + std::string(std::strerror(errno)));
    
    }
    
    tamanho_total_arquivo = info_stat.st_size;

    if (tamanho_total_arquivo > 0) {
        
        if (tamanho_total_arquivo % tamanho_bloco != 0) {
            
            std::cerr << "[Gerenciador de Blocos] Tamanho do arquivo nao e multiplo do bloco. Ajustando..." << std::endl;
            
            tamanho_total_arquivo = (tamanho_total_arquivo / tamanho_bloco + 1) * tamanho_bloco;
            
            if (ftruncate(arquivo_fd, tamanho_total_arquivo) == -1) {
                
                throw std::runtime_error("[Gerenciador de Blocos] Erro ao ajustar tamanho do arquivo: " + std::string(std::strerror(errno)));
            
            }
        
        }

        mapa_memoria = mmap(nullptr, tamanho_total_arquivo, PROT_READ | PROT_WRITE, MAP_SHARED, arquivo_fd, 0);

        if (mapa_memoria == MAP_FAILED) {

            close(arquivo_fd);
            
            throw std::runtime_error("[Gerenciador de Blocos] Erro ao mapear arquivo (mmap): " + std::string(std::strerror(errno)));
        
        }
    
    }

}

GerenciadorDeBlocos::~GerenciadorDeBlocos() {

    if (mapa_memoria != nullptr) {
        
        msync(mapa_memoria, tamanho_total_arquivo, MS_SYNC);
        
        munmap(mapa_memoria, tamanho_total_arquivo);
    
    }

    if (arquivo_fd != -1) {
    
        close(arquivo_fd);
    
    }

}

void* GerenciadorDeBlocos::getPonteiroBloco(size_t id_bloco) {
    
    size_t offset = id_bloco * tamanho_bloco;

    if (offset >= tamanho_total_arquivo || mapa_memoria == nullptr) {
    
        throw std::out_of_range("[Gerenciador de Blocos] Tentativa de ler ID de bloco invalido ou fora dos limites.");
    
    }

    blocos_lidos++;

    return (void*)(static_cast<char*>(mapa_memoria) + offset);

}

void GerenciadorDeBlocos::sincronizarBloco(size_t id_bloco) {

    void* endereco_bloco = getPonteiroBloco(id_bloco);

    if (msync(endereco_bloco, tamanho_bloco, MS_SYNC) == -1) {
        
        std::cerr << "[Gerenciador de Blocos] msync falhou para o bloco " << id_bloco << ": " << std::string(std::strerror(errno)) << std::endl;
    
    }

    blocos_escritos++;

}

size_t GerenciadorDeBlocos::alocarNovoBloco() {
    
    size_t novo_id = (tamanho_total_arquivo == 0) ? 0 : (tamanho_total_arquivo / tamanho_bloco);
    
    size_t novo_tamanho_total = (novo_id + 1) * tamanho_bloco;

    if (ftruncate(arquivo_fd, novo_tamanho_total) == -1) {
        
        throw std::runtime_error("[Gerenciador de Blocos] Erro ao estender arquivo (ftruncate): " + std::string(std::strerror(errno)));
    
    }

    if (mapa_memoria == nullptr) {
        
        mapa_memoria = mmap(nullptr, novo_tamanho_total, PROT_READ | PROT_WRITE, MAP_SHARED, arquivo_fd, 0);
        
        if (mapa_memoria == MAP_FAILED) {
            
            throw std::runtime_error("[Gerenciador de Blocos] Erro no mmap inicial ao alocar: " + std::string(std::strerror(errno)));
        
        }
    
    }
    
    else {
        
        void* novo_mapa = mremap(mapa_memoria, tamanho_total_arquivo, novo_tamanho_total, MREMAP_MAYMOVE);
        
        if (novo_mapa == MAP_FAILED) {
            
            throw std::runtime_error("Erro ao remapear arquivo (mremap): " + std::string(std::strerror(errno)));
        
        }
        
        mapa_memoria = novo_mapa;
    
    }

    tamanho_total_arquivo = novo_tamanho_total;

    void* ponteiro_novo_bloco = getPonteiroBloco(novo_id);
    
    std::memset(ponteiro_novo_bloco, 0, tamanho_bloco);
    
    sincronizarBloco(novo_id);

    return novo_id;

}