//#include "OSInfo.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "Artigo.hpp"
#include "Parser.hpp"
#include <cstring>

const int BLOCK_SIZE = 4096;
const int RECORD_SIZE = 1506;
const int RECORDS_PER_BLOCK = (BLOCK_SIZE - sizeof(int)) / RECORD_SIZE;

void serializeRecord(const Artigo& artigo, char* recordBuffer) {

    int offset = 0;

    memcpy(recordBuffer + offset, &artigo.id, sizeof(artigo.id));
    offset += sizeof(artigo.id);

    memcpy(recordBuffer + offset, &artigo.ano, sizeof(artigo.ano));
    offset += sizeof(artigo.ano);
    
    memcpy(recordBuffer + offset, &artigo.citacoes, sizeof(artigo.citacoes));
    offset += sizeof(artigo.citacoes);

    memset(recordBuffer + offset, 0, 300);
    strncpy(recordBuffer + offset, artigo.titulo.c_str(), 299);
    offset += 300;

    memset(recordBuffer + offset, 0, 150);
    strncpy(recordBuffer + offset, artigo.autores.c_str(), 149);
    offset += 150;
    
    memset(recordBuffer + offset, 0, 20);
    strncpy(recordBuffer + offset, artigo.atualizacao.c_str(), 19);
    offset += 20;

    memset(recordBuffer + offset, 0, 1024);
    strncpy(recordBuffer + offset, artigo.snippet.c_str(), 1023);
    offset += 1024;

}

int hashFunction(int id, int totalBuckets) {

    return id % totalBuckets; 

}

int main(int argc, char* argv[]) {

    if (argc < 2) {

        std::cerr << "Uso: " << argv[0] << " <caminho_para_o_csv>" << std::endl;

        return 1;

    }

    std::string inputFilePath = argv[1];
    std::ifstream inputFile(inputFilePath);

    if (!inputFile.is_open()) {

        std::cerr << "Erro: não foi possível abrir o arquivo " << inputFilePath << std::endl;

        return 1;

    }

    std::string dataFilePath = "/data/data.db";
    std::fstream dataFile(dataFilePath, std::ios::in | std::ios::out | std::ios::binary);

    if (!dataFile) {
        
        std::ofstream newFile(dataFilePath, std::ios::out | std::ios::binary);
        newFile.close();
        
        dataFile.open(dataFilePath, std::ios::in | std::ios::out | std::ios::binary);
    }

    const int TOTAL_BUCKETS = 1000;

    std::string line;
    int lineCounter = 0;

    while (std::getline(inputFile, line)) {
        if (line.empty()) {
            continue;
        }

        Artigo artigo = parseCSVLine(line);
        
        std::cout << "Processando Linha " << lineCounter++ << " -> ID: " << artigo.id << std::endl;

        if (artigo.id == -1) {
            continue;
        }

        int targetBucketIndex = hashFunction(artigo.id, TOTAL_BUCKETS);
        long bucketAddress = targetBucketIndex * BLOCK_SIZE;

        char blockBuffer[BLOCK_SIZE] = {0};
        dataFile.seekg(bucketAddress);
        dataFile.read(blockBuffer, BLOCK_SIZE);
        
        dataFile.clear();

        int recordCount;
        memcpy(&recordCount, blockBuffer, sizeof(int));

        if (recordCount < RECORDS_PER_BLOCK) {
            
            long recordPositionInBlock = sizeof(int) + (recordCount * RECORD_SIZE);
            char* recordPtr = blockBuffer + recordPositionInBlock;
            serializeRecord(artigo, recordPtr);

            recordCount++;
            memcpy(blockBuffer, &recordCount, sizeof(int));

            dataFile.seekp(bucketAddress);
            dataFile.write(blockBuffer, BLOCK_SIZE);

            std::cout << "Artigo ID " << artigo.id << " inserido no bucket " << targetBucketIndex << std::endl;

        }
        
        else {

            std::cerr << "ERRO: Bucket " << targetBucketIndex << " está cheio. Artigo ID " << artigo.id << " não inserido." << std::endl;
        
        }
        
    }

    inputFile.close();
    dataFile.close();
    std::cout << "Carga de dados concluída." << std::endl;

    return 0;
}