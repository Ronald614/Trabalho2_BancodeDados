#ifndef HASH_EXTENSIVEL_HPP
#define HASH_EXTENSIVEL_HPP
#include <optional> 

#include "gerenciador_de_blocos.hpp"
#include "bloco_de_dados.hpp"
#include "hash_extensivel_structs.hpp"
#include "Arvorebmais.hpp"

class ArquivoHashExtensivel {

    private:

        GerenciadorDeBlocos& gerenciador_dados;
        GerenciadorDeBlocos& gerenciador_dir;
        BPlusTreeInt& btree_id;

        HeaderHashExtensivel* getHeader();
        BlocoDiretorio* getBlocoDiretorio(size_t id_bloco_dir);
        size_t getPonteiroDiretorio(int hash_completo);
        void setPonteiroDiretorio(int hash_completo, size_t id_bucket);
        int getProfundidadeGlobal();
        int hash(int id, int profundidade);
        size_t alocarNovoBucketDados(int profundidade_local);
        void duplicarDiretorio();
        void dividirBucket(size_t id_bucket_cheio, Artigo artigo_novo);

    public:

        std::optional<Artigo> buscar(int id); 
        ArquivoHashExtensivel(GerenciadorDeBlocos& gm_dados, GerenciadorDeBlocos& gm_dir, BPlusTreeInt& btree_ref);
        void inicializar();
        void inserir(Artigo a);
    
};

#endif