#include "OSInfo.hpp"

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