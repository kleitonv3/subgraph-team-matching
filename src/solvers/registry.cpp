// Lista de solvers disponíveis. Para adicionar um novo algoritmo (backtracking, VF2, ...):
//   1. crie src/solvers/<nome>.cpp com uma função com a assinatura SolverFn (veja solver.hpp);
//   2. declare a função abaixo e acrescente uma linha em allSolvers();
//   3. acrescente o arquivo em SOLVER_SRCS no Makefile.
// Ele aparece automaticamente na interface e no benchmark.

#include "solver.hpp"

bool bruteForce(const Graph& pattern, const Graph& company, SearchContext& ctx, std::vector<int>& mapping);

const std::vector<SolverInfo>& allSolvers() {
    static const std::vector<SolverInfo> solvers = {
        {"brute_force", "Força bruta", bruteForce},
    };
    return solvers;
}

const SolverInfo* findSolver(const std::string& id) {
    for (const SolverInfo& solver : allSolvers()) {
        if (id == solver.id) return &solver;
    }
    return nullptr;
}
