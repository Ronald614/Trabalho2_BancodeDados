#ifndef PARSER_H
#define PARSER_H

#include "Artigo.hpp"
#include <string>

bool parseCSVLinha(const std::string& linha, Artigo& artigo_saida);
void printArtigo(const Artigo& a);

#endif