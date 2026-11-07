// Contrato entre os algoritmos de solução (solvers) e o resto do sistema (interface e benchmark).
//
// O algoritmo não desenha nada nem mede tempo. A cada candidato testado, ele chama `ctx.step(...)`,
// e o SearchContext decide o que fazer com a informação:
//   - modo visual: grava o passo num trace, que a interface reproduz depois no ritmo escolhido;
//   - modo benchmark: só conta os passos e confere o tempo limite.
//
// Regra única para o algoritmo: se `ctx.step(...)` retornar false, pare a busca e retorne.

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "graph.hpp"

// Resultado de um passo da busca.
enum class StepResult : std::uint8_t {
    Testing,  // candidato em análise, ainda sem veredito (opcional; útil para buscas parciais)
    Fail,     // candidato descartado
    Match,    // candidato é uma equipe válida
};

// Um passo gravado no trace. O mapeamento correspondente fica em Trace::mapping(i).
struct TraceStep {
    StepResult result;
    std::int16_t failU;  // aresta {failU, failV} do molde que falhou, ou vértice failU com papel errado
    std::int16_t failV;  // (failV = -1); -1 nos dois quando não se aplica
};

// Sequência de passos gravada no modo visual.
class Trace {
public:
    int patternSize() const { return k_; }
    std::size_t size() const { return steps_.size(); }
    const TraceStep& step(std::size_t i) const { return steps_[i]; }
    // map[j] = funcionário atribuído ao vértice j do molde no passo i (-1 = sem atribuição).
    const int* mapping(std::size_t i) const { return &maps_[i * static_cast<std::size_t>(k_)]; }

private:
    friend class SearchContext;
    int k_ = 0;
    std::vector<TraceStep> steps_;
    std::vector<int> maps_;
};

struct SearchOptions {
    bool recordTrace = false;           // grava os passos (modo visual)
    std::size_t traceLimit = 500000;    // acima disso, só conta os passos
    double timeLimitSeconds = 0.0;      // 0 = sem limite
    bool findAll = false;               // continua depois de achar uma equipe ("listar todas")
};

class SearchContext {
public:
    explicit SearchContext(const SearchOptions& options = SearchOptions());

    // Informa um passo da busca. Chame uma vez para cada candidato testado.
    //   map[j]  funcionário atribuído ao vértice j do molde (-1 = ainda não atribuído)
    //   k       tamanho do molde (número de posições em map)
    //   result  Testing, Fail ou Match
    //   failU, failV  em Fail: a aresta {failU, failV} do molde que não existe na empresa; ou, se a
    //                 falha foi de papel, failU = vértice do molde com papel errado e failV = -1.
    // Retorna false se a busca deve parar: tempo limite estourado, ou equipe encontrada (Match) quando
    // o modo "listar todas" está desligado.
    bool step(const int* map, int k, StepResult result, int failU = -1, int failV = -1) {
        ++steps_;
        if (options_.recordTrace) record(map, k, result, failU, failV);
        if (result == StepResult::Match) onMatch(map, k);
        if ((steps_ & kClockCheckMask) == 0) checkClock();
        return !stopped_;
    }

    // --- Resultados (usados pela interface e pelo benchmark) ---------------------------------

    std::uint64_t stepCount() const { return steps_; }
    std::uint64_t matchCount() const { return matches_; }
    bool timedOut() const { return timedOut_; }
    const Trace& trace() const { return trace_; }
    bool traceTruncated() const { return traceTruncated_; }
    // Primeira equipe informada com Match (vazio se nenhuma).
    const std::vector<int>& firstMatch() const { return firstMatch_; }

    // Marca o início da busca (para o tempo limite). Chamado por runSolver.
    void start();

private:
    // O relógio é consultado a cada 1024 passos, para não pesar na medição.
    static constexpr std::uint64_t kClockCheckMask = 1023;

    void record(const int* map, int k, StepResult result, int failU, int failV);
    void onMatch(const int* map, int k);
    void checkClock();

    SearchOptions options_;
    std::chrono::steady_clock::time_point startTime_;
    std::uint64_t steps_ = 0;
    std::uint64_t matches_ = 0;
    bool stopped_ = false;
    bool timedOut_ = false;
    bool traceTruncated_ = false;
    Trace trace_;
    std::vector<int> firstMatch_;
};

// ---------------------------------------------------------------------------------------------
// Solvers
// ---------------------------------------------------------------------------------------------

// Assinatura de um algoritmo de solução.
//   pattern  molde da equipe (k vértices)
//   company  grafo social da empresa (n vértices)
//   ctx      contexto da busca: chame ctx.step(...) a cada candidato e pare se retornar false
//   mapping  saída: se achar uma equipe, mapping[j] = funcionário escolhido para o vértice j do molde
// Retorna true se encontrou pelo menos uma equipe válida.
using SolverFn = bool (*)(const Graph& pattern, const Graph& company, SearchContext& ctx,
                          std::vector<int>& mapping);

struct SolverInfo {
    const char* id;           // usado na linha de comando e no CSV (ex.: "brute_force")
    const char* displayName;  // mostrado na interface (ex.: "Força bruta")
    SolverFn run;
};

// Todos os solvers registrados (src/solvers/registry.cpp).
const std::vector<SolverInfo>& allSolvers();
// Procura um solver pelo id. Retorna nullptr se não existir.
const SolverInfo* findSolver(const std::string& id);

// Resultado de uma execução completa.
struct RunResult {
    bool found = false;
    std::vector<int> mapping;
    std::uint64_t steps = 0;
    std::uint64_t matches = 0;
    bool timedOut = false;
    double seconds = 0.0;
    // Preenchido se o solver disse ter achado uma equipe, mas o mapeamento não é válido.
    std::string invalidReason;
};

// Executa um solver, mede o tempo e confere se a equipe devolvida é válida.
RunResult runSolver(const SolverInfo& solver, const Graph& pattern, const Graph& company, SearchContext& ctx);
