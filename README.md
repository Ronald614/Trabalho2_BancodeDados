# Trabalho 1

Implementação do banco de dados.

# Como executar

## Faz o build

```make docker-build```

## Executa o upload

```docker run --rm -v $(pwd)/data:/data tp2 ./bin/upload /data/input.csv```