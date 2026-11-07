// Formato do arquivo de resultados do benchmark (results/bench.csv).
// Escrito pela CLI (teams_bench --bench) e lido pela aba Benchmark da interface.
//
// Colunas: solver,series,preset,mode,n,k,m,seed,found,candidates,time_ms,timeout
//   series   "grow_n" (n crescente, k fixo) ou "grow_k" (k crescente, n fixo)
//   preset   id do preset (série grow_n) ou "random_k<K>" (molde gerado, série grow_k)
//   mode     "planted" ou "nosolution"
//   m        número de arestas da empresa
//   found, timeout   0 ou 1
//   time_ms  tempo da busca em milissegundos, com ponto decimal

#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct BenchRow {
    std::string solver;
    std::string series;
    std::string preset;
    std::string mode;
    int n = 0;
    int k = 0;
    int m = 0;
    std::uint64_t seed = 0;
    bool found = false;
    std::uint64_t candidates = 0;
    double timeMs = 0.0;
    bool timeout = false;
};

const char* benchCsvHeader();
std::string benchCsvLine(const BenchRow& row);

// Lê o CSV. Linhas com erro de formato são reportadas com o número da linha.
bool readBenchCsv(const std::string& path, std::vector<BenchRow>& rows, std::string& error);
