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

UPLOAD_SRCS = \
	$(SRCDIR)/upload.cpp \
	$(SRCDIR)/Parser.cpp \
	$(SRCDIR)/OSInfo.cpp \
	$(SRCDIR)/GerenciadorArquivoDados.cpp \
	$(SRCDIR)/ArquivoHashEstatico.cpp \
	$(SRCDIR)/GerenciadorIndice.cpp \
	$(SRCDIR)/Log.cpp

FINDREC_SRCS = \
	$(SRCDIR)/findrec.cpp \
    $(SRCDIR)/OSInfo.cpp \
    $(SRCDIR)/Parser.cpp \
    $(SRCDIR)/GerenciadorArquivoDados.cpp \
    $(SRCDIR)/ArquivoHashEstatico.cpp \
	$(SRCDIR)/Log.cpp

SEEK1_SRCS = \
	$(SRCDIR)/seek1.cpp \
    $(SRCDIR)/OSInfo.cpp \
    $(SRCDIR)/Parser.cpp \
    $(SRCDIR)/GerenciadorArquivoDados.cpp \
	$(SRCDIR)/GerenciadorIndice.cpp \
	$(SRCDIR)/Log.cpp

SEEK2_SRCS = \
	$(SRCDIR)/seek2.cpp \
	$(SRCDIR)/OSInfo.cpp \
	$(SRCDIR)/Parser.cpp \
	$(SRCDIR)/GerenciadorArquivoDados.cpp \
	$(SRCDIR)/GerenciadorIndice.cpp \
	$(SRCDIR)/Log.cpp

# --- Regras de Build Automáticas ---
UPLOAD_OBJS = $(patsubst $(SRCDIR)/%.cpp,$(BINDIR)/%.o,$(UPLOAD_SRCS))
FINDREC_OBJS = $(patsubst $(SRCDIR)/%.cpp,$(BINDIR)/%.o,$(FINDREC_SRCS))
SEEK1_OBJS = $(patsubst $(SRCDIR)/%.cpp,$(BINDIR)/%.o,$(SEEK1_SRCS))
SEEK2_OBJS = $(patsubst $(SRCDIR)/%.cpp,$(BINDIR)/%.o,$(SEEK2_SRCS))
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
$(BINDIR)/%.o: $(SRCDIR)/%.cpp | $(BINDIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# --- Outras Regras ---

# Esta regra será chamada como dependência pelas regras de linkagem acima.
$(BINDIR):
	mkdir -p $@

.PHONY: clean
clean:
	rm -rf $(BINDIR)

# Nome da imagem Docker
IMAGE_NAME = tp2

.PHONY: docker-build
docker-build:
	docker build -t $(IMAGE_NAME) .