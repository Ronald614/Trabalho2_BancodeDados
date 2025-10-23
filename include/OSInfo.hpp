#include <iostream>
#include <sys/stat.h>
#include <cerrno>
#include <cstring>

int obter_tamanho_bloco_fs(const char* path);