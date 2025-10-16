# Compilador C++
CXX = g++

# Flags de compilação
CXXFLAGS = -std=c++17 -Wall -Iinclude

# Diretórios
SRCDIR = src
INCDIR = include
BINDIR = bin

# --- Definição dos Programas e seus Arquivos Fonte ---
PROGRAMS = upload findrec seek1 seek2
UPLOAD_SRCS = $(SRCDIR)/upload.cpp $(SRCDIR)/OSInfo.cpp $(SRCDIR)/Parser.cpp
FINDREC_SRCS = $(SRCDIR)/findrec.cpp
SEEK1_SRCS = $(SRCDIR)/seek1.cpp
SEEK2_SRCS = $(SRCDIR)/seek2.cpp

# --- Regras de Build Automáticas ---
UPLOAD_OBJS = $(UPLOAD_SRCS:.cpp=.o)
FINDREC_OBJS = $(FINDREC_SRCS:.cpp=.o)
SEEK1_OBJS = $(SEEK1_SRCS:.cpp=.o)
SEEK2_OBJS = $(SEEK2_SRCS:.cpp=.o)
TARGETS = $(patsubst %,$(BINDIR)/%,$(PROGRAMS))

# Regra principal: 'make' ou 'make build'
.PHONY: build
build: $(TARGETS)

# Regra de "linkagem": Como criar o executável final a partir dos arquivos objeto.
# A barra vertical '|' adiciona uma "dependência de ordem".
# Isso garante que a regra $(BINDIR) execute ANTES desta, mas sem causar recompilações desnecessárias.
$(BINDIR)/upload: $(UPLOAD_OBJS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BINDIR)/findrec: $(FINDREC_OBJS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BINDIR)/seek1: $(SEEK1_OBJS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BINDIR)/seek2: $(SEEK2_OBJS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Regra de "compilação": Como transformar qualquer arquivo .cpp em .o
$(SRCDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# --- Outras Regras ---

# NOVA REGRA: Garante que o diretório de binários exista.
# Esta regra será chamada como dependência pelas regras de linkagem acima.
$(BINDIR):
	mkdir -p $@

.PHONY: clean
clean:
	rm -rf $(SRCDIR)/*.o $(BINDIR)/*

# Nome da imagem Docker
IMAGE_NAME = tp2

.PHONY: docker-build
docker-build:
	docker build -t $(IMAGE_NAME) .