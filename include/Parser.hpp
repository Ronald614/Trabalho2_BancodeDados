#ifndef PARSER_H
#define PARSER_H

#include "Artigo.hpp"
#include <string>

Artigo parseCSVLine(const std::string& line);

#endif