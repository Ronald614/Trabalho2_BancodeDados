#include "OSInfo.hpp"
#include <cmath>

/**
 * @brief Obtém o tamanho do bloco nativo do sistema de arquivos (via statvfs).
 *
 * @param path Um caminho (arquivo/diretório) no sistema de arquivos desejado.
 * @return O tamanho do bloco (em bytes) em caso de sucesso, ou -1 em caso de falha.
 */

int obter_tamanho_bloco_fs(const char* path) {
    
    struct statvfs vfs_info;

    if (statvfs(path, &vfs_info) == 0) {
    
        return vfs_info.f_bsize;
    
    }
    
    else {

        std::cerr << "Erro ao chamar statvfs para " << path << ": " << std::strerror(errno) << std::endl;

        return -1;

    }

}

/**
 * @brief Calcula o tamanho de bloco lógico (múltiplo do bloco do SO)
 * necessário para armazenar uma struct.
 * @param tamanho_bruto_struct O sizeof() da struct.
 * @param tamanho_bloco_so O tamanho do bloco do sistema de arquivos.
 * @return O tamanho lógico final para o bloco.
 */

size_t calcular_bloco_logico(size_t tamanho_bruto_struct, int tamanho_bloco_so) {
    
    if (tamanho_bloco_so <= 0) {
    
        std::cerr << "[OSInfo] Tamanho de bloco do SO inválido, usando 4096." << std::endl;
    
        tamanho_bloco_so = 4096;
    
    }
    
    if (tamanho_bruto_struct == 0) {
    
        return static_cast<size_t>(tamanho_bloco_so);
    
    }

    // Calcula quantos blocos do SO são necessários
    double num_blocos_necessarios = std::ceil(
    
        static_cast<double>(tamanho_bruto_struct) / tamanho_bloco_so
    
    );

    // Retorna o tamanho total como múltiplo do bloco do SO
    return static_cast<size_t>(num_blocos_necessarios) * tamanho_bloco_so;

}