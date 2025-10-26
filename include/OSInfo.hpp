#include <iostream>
#include <sys/statvfs.h>
#include <cerrno>
#include <cstring>
#include <cstddef>

int obter_tamanho_bloco_fs(const char* path);

size_t calcular_bloco_logico(size_t tamanho_bruto_struct, int tamanho_bloco_so);