#include <iostream>
#include <stdexcept>
#include <vector>
#include <cmath>
#include <optional>
#include <functional>
#include <unordered_set>
#include <cstring>

#include "hash_extensivel.hpp"

ArquivoHashExtensivel::ArquivoHashExtensivel(GerenciadorDeBlocos& gm_dados, GerenciadorDeBlocos& gm_dir, BPlusTreeInt& btree_ref)
    : gerenciador_dados(gm_dados),
      gerenciador_dir(gm_dir),
      btree_id(btree_ref)
{}

// =================================================================
// FUNÇÃO DE BUSCA
// =================================================================

std::optional<Artigo> ArquivoHashExtensivel::buscar(int id) {
    
    int d_global = getProfundidadeGlobal();
    int h = hash(id, d_global);
    size_t id_bucket = getPonteiroDiretorio(h);
    
    // Verifica se o id_bucket é válido antes de prosseguir
    if (id_bucket >= gerenciador_dados.obterNumeroTotalBlocos()) {
        
        std::cerr << "[hash] Ponteiro do diretório inválido para hash " << h << " (aponta para bloco " << id_bucket << ")" << std::endl;
        
        return std::nullopt;
    
    }
    
    BlocoDeDados* bucket = static_cast<BlocoDeDados*>(gerenciador_dados.getPonteiroBloco(id_bucket));

    for (size_t i = 0; i < bucket->contador_registros; ++i) {

        if (bucket->registros[i].id == id) {

            return bucket->registros[i];

        }

    }

    return std::nullopt;

}

// =================================================================
// FUNÇÕES AUXILIARES
// =================================================================

HeaderHashExtensivel* ArquivoHashExtensivel::getHeader() {
    
    if (gerenciador_dir.obterNumeroTotalBlocos() == 0) {
        
        throw std::runtime_error("[hash] Tentativa de acessar header antes da inicialização.");
    
    }
    
    return static_cast<HeaderHashExtensivel*>(gerenciador_dir.getPonteiroBloco(0));

}

int ArquivoHashExtensivel::getProfundidadeGlobal() {
    
    return getHeader()->profundidade_global;

}

BlocoDiretorio* ArquivoHashExtensivel::getBlocoDiretorio(size_t id_bloco_dir) {
    
    if (id_bloco_dir == 0) {
    
        throw std::invalid_argument("[hash] Tentativa de acessar Bloco 0 (Header) como Bloco de Diretorio.");
    
    }

    if (id_bloco_dir >= gerenciador_dir.obterNumeroTotalBlocos()) {
    
        throw std::out_of_range("[hash] Tentativa de acessar bloco de diretório inválido: " + std::to_string(id_bloco_dir));
    
    }
    
    return static_cast<BlocoDiretorio*>(gerenciador_dir.getPonteiroBloco(id_bloco_dir));

}

int ArquivoHashExtensivel::hash(int id, int profundidade) {
    
    if (profundidade <= 0) {
        
        return 0;
    
    }
    
    profundidade = std::min(profundidade, 30);

    size_t hashed_id = std::hash<int>{}(id);
    size_t mascara = (size_t(1) << profundidade) - 1;
    
    return static_cast<int>(hashed_id & mascara);

}

size_t ArquivoHashExtensivel::getPonteiroDiretorio(int hash_completo) {
    
    int d_global = getProfundidadeGlobal();
    
    if (d_global <= 0 || d_global > 30) {
        
        throw std::runtime_error("[hash] Profundidade global inválida ou grande demais ao buscar ponteiro: " + std::to_string(d_global));
    
    }
    
    int tamanho_diretorio = 1 << d_global;

    if (hash_completo < 0 || hash_completo >= tamanho_diretorio) {
        
        std::cerr << "[hash] Hash fora dos limites do diretório! Hash: " << hash_completo << ", Tamanho: " << tamanho_diretorio << std::endl;
        
        throw std::out_of_range("[hash] Hash fora dos limites do diretório: " + std::to_string(hash_completo));
    
    }
    
    size_t id_bloco_dir = 1 + (hash_completo / PONTEIROS_POR_BLOCO_DIR);
    size_t offset_no_bloco = hash_completo % PONTEIROS_POR_BLOCO_DIR;
    
    BlocoDiretorio* dir = getBlocoDiretorio(id_bloco_dir);
    
    size_t id_bucket_lido = dir->ponteiros[offset_no_bloco];
    
    if (id_bucket_lido == static_cast<size_t>(-1)) {
        
        std::cerr << "[hash] Ponteiro não inicializado encontrado no diretório! Hash: " << hash_completo << std::endl;
        
        throw std::runtime_error("[hash] Ponteiro inválido (-1) encontrado no diretório.");
    
    }
    
    return id_bucket_lido;

}

void ArquivoHashExtensivel::setPonteiroDiretorio(int hash_completo, size_t id_bucket) {
    
    int d_global = getProfundidadeGlobal();
    
    if (d_global <= 0 || d_global > 30) {
    
        throw std::runtime_error("[hash] Profundidade global inválida ao setar ponteiro: " + std::to_string(d_global));
    
    }

    int tamanho_diretorio = 1 << d_global;

    if (hash_completo < 0 || hash_completo >= tamanho_diretorio) {
        
        throw std::out_of_range("[hash] Tentativa de setar ponteiro fora dos limites do diretório: " + std::to_string(hash_completo));
    
    }
    
    if (id_bucket == static_cast<size_t>(-1)) {
        
        throw std::invalid_argument("[hash] Tentativa de setar ponteiro inválido (-1) no diretório.");
    
    }
    
    size_t id_bloco_dir = 1 + (hash_completo / PONTEIROS_POR_BLOCO_DIR);
    size_t offset_no_bloco = hash_completo % PONTEIROS_POR_BLOCO_DIR;
    
    BlocoDiretorio* dir = getBlocoDiretorio(id_bloco_dir);
    
    dir->ponteiros[offset_no_bloco] = id_bucket;
    gerenciador_dir.sincronizarBloco(id_bloco_dir);

}

// =================================================================
// FUNÇÕES DE LÓGICA (Inicialização e Alocação)
// =================================================================

size_t ArquivoHashExtensivel::alocarNovoBucketDados(int profundidade_local) {

    HeaderHashExtensivel* header = getHeader();
    
    size_t novo_id_bucket = header->proximo_bloco_livre_dados;
    header->proximo_bloco_livre_dados++;
    
    gerenciador_dir.sincronizarBloco(0);
    
    if (novo_id_bucket >= gerenciador_dados.obterNumeroTotalBlocos()) {
    
        gerenciador_dados.alocarNovoBloco();
    
    }
    
    BlocoDeDados* bucket = static_cast<BlocoDeDados*>(gerenciador_dados.getPonteiroBloco(novo_id_bucket));
    
    bucket->profundidade_local = profundidade_local;
    bucket->contador_registros = 0;
    
    for(size_t i = 0; i < CAPACIDADE_BUCKET; ++i) {
    
        bucket->registros[i] = Artigo{};
    
    }
    
    gerenciador_dados.sincronizarBloco(novo_id_bucket);
    
    return novo_id_bucket;

}

void ArquivoHashExtensivel::inicializar() {

    if (gerenciador_dir.obterNumeroTotalBlocos() == 0) {

        std::cout << "Inicializando nova estrutura de Hashing Extensivel..." << std::endl;

        gerenciador_dir.alocarNovoBloco();

        HeaderHashExtensivel* header = getHeader();

        header->profundidade_global = 1;
        header->proximo_bloco_livre_dados = 0;

        gerenciador_dir.sincronizarBloco(0);

        size_t id_bucket_0 = alocarNovoBucketDados(1);
        size_t id_bucket_1 = alocarNovoBucketDados(1);

        gerenciador_dir.alocarNovoBloco();

        BlocoDiretorio* dir = getBlocoDiretorio(1);

        dir->ponteiros[0] = id_bucket_0;
        dir->ponteiros[1] = id_bucket_1;

        for (size_t i = 2; i < PONTEIROS_POR_BLOCO_DIR; ++i) {

            dir->ponteiros[i] = static_cast<size_t>(-1);

        }

        gerenciador_dir.sincronizarBloco(1);

        std::cout << "Hashing Extensivel inicializado. (d=1, 2 buckets)" << std::endl;

    }
    
    else {
        
        try {
        
            int d_global = getProfundidadeGlobal();
        
            std::cout << "Arquivo de Hashing Extensivel ja inicializado. (d=" << d_global << ")" << std::endl;
        
        }
        
        catch (const std::exception& e) {
            
            std::cerr << "Erro: Arquivo de diretório existe mas parece corrompido. " << e.what() << std::endl;
            
            throw;
        
        }
    
    }

}


// =================================================================
// FUNÇÃO INSERIR
// =================================================================

void ArquivoHashExtensivel::inserir(Artigo a) {

    int d_global = getProfundidadeGlobal();
    int h = hash(a.id, d_global);
    
    size_t id_bucket = getPonteiroDiretorio(h);
    
    if (id_bucket >= gerenciador_dados.obterNumeroTotalBlocos()) {
        
        std::cerr << "[hash] Diretório aponta para bucket inválido (" << id_bucket << ") para ID " << a.id << std::endl;
        throw std::runtime_error("[hash] Ponteiro do diretório inválido.");
    
    }
    
    BlocoDeDados* bucket = static_cast<BlocoDeDados*>(gerenciador_dados.getPonteiroBloco(id_bucket));

    if (bucket->contador_registros < CAPACIDADE_BUCKET) {
        
        bucket->registros[bucket->contador_registros] = a;
        bucket->contador_registros++;
        gerenciador_dados.sincronizarBloco(id_bucket);

        try {
        
            btree_id.insert(a.id, id_bucket);
        
        }
        
        catch (const std::exception& e) {
            
            std::cerr << "Erro ao inserir ID " << a.id << " na B+Tree (Caso 1): " << e.what() << std::endl;
            
        }
        
        return;
    
    }

    dividirBucket(id_bucket, a);

}


// =================================================================
// FUNÇÃO DIVIDIR BUCKET
// =================================================================

void ArquivoHashExtensivel::dividirBucket(size_t id_bucket_cheio, Artigo artigo_novo) {

    int d_local;

    std::unordered_set<int> ids_antigos;
    std::vector<Artigo> registros_originais;

    registros_originais.reserve(CAPACIDADE_BUCKET);

    {
        if (id_bucket_cheio >= gerenciador_dados.obterNumeroTotalBlocos()) {
        
            throw std::runtime_error("[hash] id_bucket_cheio inválido.");
        
        }
        
        BlocoDeDados* bucket_leitura_rapida = static_cast<BlocoDeDados*>(gerenciador_dados.getPonteiroBloco(id_bucket_cheio));
        d_local = bucket_leitura_rapida->profundidade_local;
         
        if (d_local < 0 || d_local > getProfundidadeGlobal()) {
             
            throw std::runtime_error("[hash] Profundidade local inválida detectada no bucket " + std::to_string(id_bucket_cheio) + ": d_local=" + std::to_string(d_local) + ", d_global=" + std::to_string(getProfundidadeGlobal()));
        
        }
        
        for(size_t i = 0; i < bucket_leitura_rapida->contador_registros; ++i) {
        
            ids_antigos.insert(bucket_leitura_rapida->registros[i].id);
            registros_originais.push_back(bucket_leitura_rapida->registros[i]);
        
        }
    
    }

    int nova_d_local = d_local + 1;
    
    if (nova_d_local > 30) {
    
        throw std::runtime_error("[hash] Profundidade local máxima excedida durante split.");
    
    }
    
    size_t id_novo_bucket = alocarNovoBucketDados(nova_d_local);

    BlocoDeDados* bucket_cheio = static_cast<BlocoDeDados*>(gerenciador_dados.getPonteiroBloco(id_bucket_cheio));
    BlocoDeDados* novo_bucket = static_cast<BlocoDeDados*>(gerenciador_dados.getPonteiroBloco(id_novo_bucket));

    bucket_cheio->profundidade_local = nova_d_local;

    std::vector<Artigo> todos_registros = registros_originais;

    todos_registros.push_back(artigo_novo);

    bucket_cheio->contador_registros = 0;

    for (const auto& art : todos_registros) {

        int h_novo = hash(art.id, nova_d_local);
        bool vai_para_novo_bucket = (h_novo & (1 << d_local));
        bool eh_antigo = ids_antigos.count(art.id);

        try {

            if (vai_para_novo_bucket) {

                if (novo_bucket->contador_registros < CAPACIDADE_BUCKET) {

                    novo_bucket->registros[novo_bucket->contador_registros++] = art;

                }
                
                else {
                    
                    std::cerr << "[hash] Novo bucket encheu durante redistribuição! ID: " << art.id << std::endl;
                    
                    throw std::runtime_error("[hash] Overflow inesperado no novo bucket durante split.");
                
                }

                if (eh_antigo) {

                    btree_id.remove(art.id);
                    btree_id.insert(art.id, id_novo_bucket);
                
                }
                
                else {
                
                    btree_id.insert(art.id, id_novo_bucket);
                
                }

            }
            
            else {
                
                if (bucket_cheio->contador_registros < CAPACIDADE_BUCKET) {
                    
                    bucket_cheio->registros[bucket_cheio->contador_registros++] = art;
                
                }
                
                else {
                    
                    std::cerr << "ERRO CRÍTICO: Bucket antigo encheu durante redistribuição! ID: " << art.id << std::endl;
                    
                    throw std::runtime_error("Overflow inesperado no bucket antigo durante split.");
                
                }

                if (!eh_antigo) {
                    
                    btree_id.insert(art.id, id_bucket_cheio);
                
                }
            
            }
        }
        
        catch (const std::exception& e) {
            
            std::cerr << "Erro ao atualizar ID " << art.id << " na B+Tree (Split): " << e.what() << std::endl;
            
        }
    
    }

    gerenciador_dados.sincronizarBloco(id_bucket_cheio);
    gerenciador_dados.sincronizarBloco(id_novo_bucket);

    int d_global_antes = getProfundidadeGlobal();
    
    if (nova_d_local > d_global_antes) {

        duplicarDiretorio();
        
        if (getProfundidadeGlobal() != d_global_antes + 1) {
            
            throw std::runtime_error("Erro: Duplicação do diretório falhou em incrementar a profundidade global.");
        
        }
    
    }

    int d_global_atual = getProfundidadeGlobal();
    int tamanho_diretorio = 1 << d_global_atual;
    int hash_base_antigo = hash(registros_originais[0].id, d_local);

    for (int i = 0; i < tamanho_diretorio; ++i) {
        
        if ((i & ((1 << d_local) - 1)) == hash_base_antigo) {
            
            if (i & (1 << d_local)) {
            
                setPonteiroDiretorio(i, id_novo_bucket);
            
            }
            
            else {
            
                setPonteiroDiretorio(i, id_bucket_cheio);
            
            }
        
        }
    
    }

}


// =================================================================
// FUNÇÃO DUPLICAR DIRETÓRIO
// =================================================================

void ArquivoHashExtensivel::duplicarDiretorio() {
    
    std::cout << "Duplicando diretorio..." << std::endl;
    
    HeaderHashExtensivel* header = getHeader();
    int d_antigo = header->profundidade_global;

    if (d_antigo >= 30) {
    
        throw std::runtime_error("[hash] Profundidade global máxima atingida.");
    
    }
    
    int tamanho_antigo = 1 << d_antigo;
    int d_novo = d_antigo + 1;
    int tamanho_novo = 1 << d_novo;

    header->profundidade_global = d_novo;
    gerenciador_dir.sincronizarBloco(0);

    size_t blocos_dir_necessarios = (tamanho_novo + PONTEIROS_POR_BLOCO_DIR - 1) / PONTEIROS_POR_BLOCO_DIR;
    size_t blocos_totais_necessarios = blocos_dir_necessarios + 1;
    size_t blocos_dir_atuais_total = gerenciador_dir.obterNumeroTotalBlocos();

    for (size_t i = blocos_dir_atuais_total; i < blocos_totais_necessarios; ++i) {

        gerenciador_dir.alocarNovoBloco();

        void* novo_bloco_ptr = gerenciador_dir.getPonteiroBloco(i);
        memset(novo_bloco_ptr, 0, gerenciador_dir.obterTamanhoBloco());

        gerenciador_dir.sincronizarBloco(i);

    }

    for (int i = 0; i < tamanho_antigo; ++i) {

        size_t ponteiro = getPonteiroDiretorio(i);
        
        if (ponteiro == static_cast<size_t>(-1)) {
            
            std::cerr << "Aviso: Encontrado ponteiro inválido (-1) ao duplicar diretório na posição " << i << std::endl;
             
        }
 
        setPonteiroDiretorio(i + tamanho_antigo, ponteiro);
 
    }

    std::cout << "Diretorio duplicado. Nova profundidade global: " << d_novo << std::endl;

}