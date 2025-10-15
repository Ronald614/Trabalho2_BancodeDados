#include <iostream>
#include <sys/statvfs.h>
#include <cerrno>
#include <cstring>

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

int main() {
    
    const char* data_dir = "/data"; 

    int fs_block_size = obter_tamanho_bloco_fs(data_dir);

    if (fs_block_size != -1) {
 
        std::cout << "Tamanho do Bloco do Sistema de Arquivos (" << data_dir << "): " << fs_block_size << " bytes" << std::endl;
 
    }

    return 0;
}