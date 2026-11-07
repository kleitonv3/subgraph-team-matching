// Séries do benchmark (SPEC, Fase 6).
//
//   grow_n  n crescente, k fixo: para cada preset, empresas geradas cada vez maiores.
//           Responde "até que tamanho de empresa a força bruta é viável".
//   grow_k  k crescente, n fixo: moldes gerados cada vez maiores numa empresa de tamanho fixo.
//           Mostra a explosão exponencial (com k fixo, a força bruta cresce como n^k).
//
// Cada série roda nos modos plantada e sem solução e para no primeiro tamanho em que alguma
// execução estoura o tempo limite.

#pragma once

#include <iosfwd>
#include <string>
#include <vector>

#include "dataset.hpp"

struct BenchConfig {
    double timeoutSeconds = 10.0;
    int plantedReps = 3;          // repetições (seeds 1..R) no modo plantada
    double averageDegree = 6.0;
    int maxN = 4096;              // maior empresa da série grow_n
    int growKCompanySize = 20;    // n fixo da série grow_k
    int maxK = 12;                // maior molde da série grow_k
    bool runGrowN = true;
    bool runGrowK = true;
    std::string onlyPreset;       // vazio = todos
    std::string onlySolver;       // vazio = todos os solvers registrados
};

// Roda as séries, escrevendo as linhas do CSV em `csv` e o progresso em `log`.
// Retorna 0 se tudo correu bem; 1 se algum solver não testou nenhum candidato ou devolveu equipe inválida.
int runBenchmark(const BenchConfig& config, const std::vector<Preset>& presets, std::ostream& csv, std::ostream& log);
