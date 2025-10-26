#include <stdexcept>
#include <iostream>
#include <cstring>

#include "ArquivoHashEstatico.hpp"

ArquivoHashEstatico::ArquivoHashEstatico(GerenciadorArquivoDados& gm_dados, int num_buckets) : gerenciador_dados(gm_dados), NUM_BUCKETS_PRIMARIOS(num_buckets) {
    
    if (num_buckets <= 0) {
        
        throw std::invalid_argument("[Hash Estático] O número de buckets primários deve ser positivo.");
    
    }

}

int ArquivoHashEstatico::hash(int id) {

    long long hash_val = static_cast<long long>(id);

    if (hash_val < 0) {

        hash_val = -hash_val;

    }

    return static_cast<int>(hash_val % NUM_BUCKETS_PRIMARIOS);

}

size_t ArquivoHashEstatico::alocarNovoBucket() {
    
    size_t novo_id = gerenciador_dados.alocarNovoBloco();
    
    BlocoDeDados* bucket = static_cast<BlocoDeDados*>(gerenciador_dados.getPonteiroBloco(novo_id));
    
    bucket->proximo_bloco_overflow = -1;
    
    return novo_id;
    
}

void ArquivoHashEstatico::inicializar() {
    
    if (gerenciador_dados.obterNumeroTotalBlocos() == 0) {
        
        std::cout << "[Hash Estático] Inicializando com " << NUM_BUCKETS_PRIMARIOS << " buckets primários..." << std::endl;
        
        gerenciador_dados.alocarBlocosEmMassa(NUM_BUCKETS_PRIMARIOS);

        std::cout << "[Hash Estático] Configurando ponteiros de overflow..." << std::endl;
        
        // Configura os ponteiros overflow dos buckets.
        for (int i = 0; i < NUM_BUCKETS_PRIMARIOS; ++i) {
            
            BlocoDeDados* bucket = static_cast<BlocoDeDados*>(gerenciador_dados.getPonteiroBloco(i));
            
            bucket->proximo_bloco_overflow = -1;
        
        }
        
        gerenciador_dados.sincronizarArquivoInteiro();
    
    }
    
    else {
        
        std::cout << "[Hash Estático] Arquivo de dados já inicializado." << std::endl;
    
    }

}

size_t ArquivoHashEstatico::inserir(const Artigo& a) {
    
    int id_bucket_atual = hash(a.id);

    while (true) {
        
        BlocoDeDados* bucket = static_cast<BlocoDeDados*>(gerenciador_dados.getPonteiroBloco(id_bucket_atual));

        // Caso 1: Bucket atual tem espaço livre.
        if (bucket->contador_registros < CAPACIDADE_BUCKET) {
            
            bucket->registros[bucket->contador_registros] = a;
            bucket->contador_registros++;
            
            return id_bucket_atual; 

        }

        // Caso 2: Bucket está cheio e não tem próximo bloco alocado.
        // Este é o bloco que trata a colisão alocando um novo bloco de overflow.
        if (bucket->proximo_bloco_overflow == -1) {
            
            size_t novo_id_overflow = alocarNovoBucket();

            BlocoDeDados* bucket_atualizado = static_cast<BlocoDeDados*>(gerenciador_dados.getPonteiroBloco(id_bucket_atual));
            
            bucket_atualizado->proximo_bloco_overflow = novo_id_overflow; 
            
            BlocoDeDados* bucket_overflow = static_cast<BlocoDeDados*>(gerenciador_dados.getPonteiroBloco(novo_id_overflow));
            
            bucket_overflow->registros[bucket_overflow->contador_registros] = a;
            bucket_overflow->contador_registros++;
            
            return novo_id_overflow;

        }

        // Caso 3: Bucket está cheio, vai para o próximo da cadeia
        id_bucket_atual = bucket->proximo_bloco_overflow;
    
    }

}

std::optional<Artigo> ArquivoHashEstatico::buscar(int id) {
    
    int id_bucket_atual = hash(id);

    // Percorre a cadeia de overflow
    while (id_bucket_atual != -1) {
        
        BlocoDeDados* bucket = static_cast<BlocoDeDados*>(gerenciador_dados.getPonteiroBloco(id_bucket_atual));

        // Procura linearmente o ID dentro do bucket atual
        for (size_t i = 0; i < bucket->contador_registros; ++i) {
        
            if (bucket->registros[i].id == id) {
        
                // Encontrou: retorna o artigo.
                return bucket->registros[i];
        
            }
        
        }
        
        // Não encontrou no bucket atual, avança para o próximo
        id_bucket_atual = bucket->proximo_bloco_overflow;
    
    }

    // Percorreu toda a cadeia e não encontrou: retorna vazio.
    return std::nullopt;
    
}