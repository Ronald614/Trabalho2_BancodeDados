# Trabalho 1

Implementação do banco de dados.

# Como executar

## Faz o build

```make docker-build```

## Executa o upload

### Upload

``docker compose run --rm upload input.csv``

### findrec

``docker compose run --rm findrec id``

### seek1

``docker compose run --rm seek1 id``

### seek2

``docker compose run --rm seek2 "Um Titulo Exato"``

# Desligar o docker

``docker compose down -v``