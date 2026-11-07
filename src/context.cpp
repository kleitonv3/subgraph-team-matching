#include <algorithm>
#include <string>

#include "solver.hpp"

SearchContext::SearchContext(const SearchOptions& options) : options_(options) {
    startTime_ = std::chrono::steady_clock::now();
}

void SearchContext::start() { startTime_ = std::chrono::steady_clock::now(); }

void SearchContext::record(const int* map, int k, StepResult result, int failU, int failV) {
    if (trace_.steps_.size() >= options_.traceLimit) {
        traceTruncated_ = true;
        return;
    }
    trace_.k_ = k;
    trace_.steps_.push_back({result, static_cast<std::int16_t>(failU), static_cast<std::int16_t>(failV)});
    trace_.maps_.insert(trace_.maps_.end(), map, map + k);
}

void SearchContext::onMatch(const int* map, int k) {
    ++matches_;
    if (firstMatch_.empty()) firstMatch_.assign(map, map + k);
    if (!options_.findAll) stopped_ = true;
}

void SearchContext::checkClock() {
    if (options_.timeLimitSeconds <= 0.0 || timedOut_) return;
    const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - startTime_;
    if (elapsed.count() >= options_.timeLimitSeconds) {
        timedOut_ = true;
        stopped_ = true;
    }
}

namespace {

// Confere a equipe devolvida pelo solver contra a definição de solução (SPEC, seção 2).
// Não é usada pelos solvers: serve só para validar o resultado depois da busca.
bool validateMapping(const Graph& pattern, const Graph& company, const std::vector<int>& mapping,
                     std::string& reason) {
    const int k = pattern.vertexCount();
    const int n = company.vertexCount();
    if (static_cast<int>(mapping.size()) != k) {
        reason = "o mapeamento tem " + std::to_string(mapping.size()) + " posições, mas o molde tem " +
                 std::to_string(k) + " vértices";
        return false;
    }
    for (int j = 0; j < k; ++j) {
        if (mapping[j] < 0 || mapping[j] >= n) {
            reason = "a posição " + std::to_string(j) + " aponta para um funcionário inexistente (" +
                     std::to_string(mapping[j]) + ")";
            return false;
        }
    }
    std::vector<int> sorted = mapping;
    std::sort(sorted.begin(), sorted.end());
    if (std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end()) {
        reason = "o mesmo funcionário foi usado em duas posições";
        return false;
    }
    for (int j = 0; j < k; ++j) {
        if (pattern.role(j) != company.role(mapping[j])) {
            reason = "o papel da posição " + std::to_string(j) + " não bate";
            return false;
        }
    }
    for (const auto& edge : pattern.edges()) {
        if (!company.hasEdge(mapping[edge.first], mapping[edge.second])) {
            reason = "a aresta " + std::to_string(edge.first) + "-" + std::to_string(edge.second) +
                     " do molde não existe na empresa";
            return false;
        }
    }
    return true;
}

}  // namespace

RunResult runSolver(const SolverInfo& solver, const Graph& pattern, const Graph& company, SearchContext& ctx) {
    RunResult result;
    std::vector<int> mapping;

    ctx.start();
    const auto begin = std::chrono::steady_clock::now();
    result.found = solver.run(pattern, company, ctx, mapping);
    const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - begin;

    result.seconds = elapsed.count();
    result.steps = ctx.stepCount();
    result.matches = ctx.matchCount();
    result.timedOut = ctx.timedOut();
    if (result.found) {
        result.mapping = mapping;
        validateMapping(pattern, company, mapping, result.invalidReason);
    }
    return result;
}
