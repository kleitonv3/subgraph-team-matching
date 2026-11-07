// Gerador de instâncias para o benchmark (SPEC, seção 5).
//
// Todos os geradores usam um gerador de números aleatórios próprio (e não o <random>), para que a
// mesma seed produza o mesmo grafo em qualquer compilador e sistema operacional.

#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "graph.hpp"

// Gerador pseudoaleatório pequeno e determinístico (SplitMix64).
class Random {
public:
    explicit Random(std::uint64_t seed) : state_(seed) {}
    std::uint64_t next();
    // Número real uniforme em [0, 1).
    double uniform();
    // Inteiro uniforme em [0, bound).
    int below(int bound);

private:
    std::uint64_t state_;
};

// Peso relativo de cada papel ao sortear os papéis dos funcionários (índice = Role).
using RoleWeights = std::array<double, kRoleCount>;
// Distribuição padrão: mais Devs, poucos Arquitetos.
RoleWeights defaultRoleWeights();

enum class CompanyMode {
    Random,      // só o grafo aleatório
    Planted,     // embute o molde em posições aleatórias: existe pelo menos uma solução
    NoSolution,  // garante que não existe solução: a busca percorre tudo (pior caso)
};

// Nome usado na linha de comando e no CSV ("random", "planted", "nosolution").
const char* companyModeId(CompanyMode mode);
bool parseCompanyMode(const std::string& text, CompanyMode& mode);

struct CompanyParams {
    int n = 100;                  // número de funcionários
    double averageDegree = 6.0;   // colaboradores por pessoa, em média: p = d / (n - 1)
    std::uint64_t seed = 1;
    CompanyMode mode = CompanyMode::Random;
    RoleWeights roles = defaultRoleWeights();
};

// Gera uma empresa. Nos modos Planted e NoSolution, `pattern` é obrigatório; em todos os modos, se
// `pattern` for informado, a empresa tem pelo menos tantos funcionários de cada papel quanto o molde
// exige (para que a falta de papéis nunca resolva o problema sozinha).
// Retorna false e preenche `error` se os parâmetros forem inválidos.
bool generateCompany(const CompanyParams& params, const Graph* pattern, Graph& company, std::string& error);

struct PatternParams {
    int k = 4;                         // número de vértices do molde
    double extraEdgeProbability = 0.3; // chance de cada aresta extra, além da árvore que garante conexão
    std::uint64_t seed = 1;
    RoleWeights roles = defaultRoleWeights();
};

// Gera um molde conexo aleatório com k vértices, já com posições em círculo para a interface.
bool generatePattern(const PatternParams& params, Graph& pattern, std::string& error);
