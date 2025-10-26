# Trabalho Prático 2 - Banco de Dados I

Trabalho para implementar um sistema de armazenamento e consulta de dados em memória secundária, usando Hashing Estático e ÁrvoreB+.

## Compilação

### Compilação Local

Para compilar os binários localmente:

```bash
make build
```

Os binários serão gerados no diretório `bin/`.

### Compilação via Docker

Para construir a imagem Docker:

```bash
make docker-build
```

## Execução

O `docker-compose.yml` está configurado para montar o diretório local `./data` no diretório `/data` dentro do contêiner.

### 1\. `upload`

Carrega os dados de um arquivo CSV armazenado em `./data` e constrói os arquivos de dados (`.dat`) e índices (`.idx`).

Sintaxe: 

``docker compose run --rm upload <nome_do_arquivo>.csv``

```bash
docker compose run --rm upload artigo.csv
```

### 2\. `findrec`

Busca um registro diretamente no arquivo de dados hashing usando o `ID`.

Sintaxe:

`docker compose run --rm findrec <ID>`

```bash
docker compose run --rm findrec 7
```

### 3\. `seek1`

Busca um registro usando o índice primário usando a árvore B+ com o campo `ID`.

Sintaxe: 

``docker compose run --rm seek1 <ID>``

```bash
docker compose run --rm seek1 7
```

### 4\. `seek2`

Busca um registro usando o índice primário usando a árvore B+ com o campo `Titulo`.

É necessário usar aspas duplas se o título tiver espaços.

Sintaxe: 

``docker compose run --rm seek2 "<Titulo Exato>"``

```bash
docker compose run --rm seek2 "Um Titulo Exato"
```

-----

### Controlando o nível de log

É possível controlar a verbosidade dos programas usando a variável de ambiente `LOG_LEVEL` usando as seguintes flags:

  * `error`: Mostra apenas erros fatais.
  * `warn`: Mostra erros e avisos.
  * `info`: Mostra o fluxo normal, progresso e estatísticas finais.
  * `debug`: Mostra todas as mensagens, incluindo detalhes técnicos.

Para definir a variável, use a flag `-e` no comando `docker compose run`:

Sintaxe: 

``docker compose run --rm -e LOG_LEVEL=<flag escolhida> seek1 1``

```bash
docker compose run --rm -e LOG_LEVEL=debug seek1 123
```

Se nenhuma for escolhida, a flag ``info`` é a padrão.

-----

## Layout dos arquivos de dados em `./data/db`

O programa `upload` gera os seguintes arquivos de banco de dados no diretório `./data/db`, equivalente ao `/data/db` dentro do contêiner:

  * `db.meta`: Arquivo de metadados. Armazena os tamanhos de bloco de dados e de índice definidos durante o `upload`.
  * `artigos.dat`: Arquivo de dados principal, organizado por Hashing Estático.
  * `btree_id.idx`: Arquivo de índice primário Árvore B+ para o campo `ID`.
  * `btree_titulo.idx`: Arquivo de índice secundário Árvore B+ para o campo `Titulo`.

## Exemplo de Entrada e Saída

Exemplo de uma busca `findrec` com a saída padrão (`LOG_LEVEL=info`).

**Comando de entrada:**

```bash
docker compose run --rm findrec 4
```

**Resultado:**

```
[INFO] --- Iniciando Busca (findrec) ---
[INFO] Buscando ID: 4
[INFO] Arquivo de Dados (Hash): /data/db/artigos.dat
[INFO] --- Registro Encontrado ---
-------------------------------------
ID: 4
Ano: 2013
Citacoes: 1
Titulo: Poster: Real time hand pose recognition with depth sensors for mixed reality interfaces.
Autores: Byungkyu Kang|Mathieu Rodrigue|Tobias H&ouml;llerer|Hwasup Lim
Atualizacao: 2016-10-03 21:38:17
Snippet: "Poster: Real time hand pose recognition with depth sensors for mixed reality interfaces. B Kang, M Rodrigue, T HÃ¶llerer - 3D User Interfaces (3DUI) , 2013 - ieeexplore.ieee.org. ABSTRACT We present a method for predicting articulated hand poses in realtime with a [...]"
-------------------------------------
[INFO] 
[INFO] --- Estatísticas da Operação (findrec) ---
[INFO] Tempo total de execução: 0 ms
[INFO] Arquivo de Dados: /data/db/artigos.dat
[INFO]   - Blocos lidos (Dados): 2
[INFO]   - Total de blocos (Dados): 4
```

## Limpeza

Para desligar os contêineres e remover os volumes anônimos, não afeta `./data`:

```bash
docker compose down -v
```