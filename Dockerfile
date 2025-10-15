# Imagem base.
FROM debian:bookworm-slim

# Instalar as ferramentas necessárias para compilação (g++, make).
RUN apt-get update && apt-get install -y --no-install-recommends g++ make && rm -rf /var/lib/apt/lists/*

# Definir o diretório de trabalho dentro do contêiner.
WORKDIR /app

# Copiar os arquivos de código-fonte e o Makefile.
# Copiar o Makefile primeiro para aproveitar o cache do Docker.
# Se o Makefile não mudar, não precisamos copiar os fontes novamente.
COPY Makefile .
COPY ./src ./src
COPY ./include ./include

# Compilar os programas usando o Makefile, resultando na criação dos binários no diretório.
RUN make build

# Criar um volume para persistir os dados.
# O diretório /data será usado para entrada (CSV) e saída (arquivos do BD).
VOLUME /data

# Comando padrão para quando o contêiner iniciar sem um comando específico.
# Apenas lista os binários compilados para mostrar que está pronto.
CMD ["/bin/bash", "-c", "echo 'Imagem pronta. Use docker run para executar um programa. Binários disponíveis:'; ls -1 bin/"]