# Compilador C++.
CXX = g++

# Flags de compilação.
# -Iinclude diz ao compilador para procurar arquivos de cabeçalho na pasta "include/".
CXXFLAGS = -std=c++17 -Wall -Iinclude

# Diretórios.
SRCDIR = src
BINDIR = bin

# Garante que a pasta de binários exista.
$(shell mkdir $(BINDIR))

# Nomes dos programas.
PROGRAMS = upload findrec seek1 seek2

# Gera a lista de arquivos de origem (.cpp).
SOURCES = $(patsubst %,$(SRCDIR)/%.cpp,$(PROGRAMS))

# Gera a lista de arquivos de destino (binários).
TARGETS = $(patsubst %,$(BINDIR)/%,$(PROGRAMS))

# Nome da imagem Docker.
IMAGE_NAME = tp2

# Regra padrão: 'make' ou 'make build' compilará tudo.
.PHONY: build
build: $(TARGETS)

# Regra para compilar um único programa.
$(BINDIR)/%: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

# Regra para limpar os binários compilados
.PHONY: clean
clean:
	rm -rf $(BINDIR)/*

# --- Comandos Docker ---

# Constrói a imagem Docker.
.PHONY: docker-build
docker-build:
	docker build -t $(IMAGE_NAME) .