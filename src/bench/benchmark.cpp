#include "benchmark.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <ostream>
#include <string>
#include <vector>

#include "bench_csv.hpp"
#include "format.hpp"
#include "generator.hpp"
#include "solver.hpp"

namespace {

// Tamanhos de empresa da série grow_n (crescimento de ~1,5x)
const int kCompanySizes[] = {4,   6,   8,   12,  16,  24,  32,   48,   64,   96,   128,
                             192, 256, 384, 512, 768, 1024, 1536, 2048, 3072, 4096};

struct SizeOutcome {
    bool anyTimeout = false;
    bool problem = false;  // solver sem candidatos ou com equipe inválida
};

template <typename T>
T median(std::vector<T> values) {
    std::sort(values.begin(), values.end());
    return values[values.size() / 2];
}

// Roda um tamanho (todas as repetições), grava as linhas e imprime uma linha de progresso.
SizeOutcome runSize(const BenchConfig& config, const SolverInfo& solver, const char* series, const std::string& presetId,
                    const Graph& pattern, int n, CompanyMode mode, std::ostream& csv, std::ostream& log) {
    SizeOutcome outcome;
    const int reps = mode == CompanyMode::Planted ? std::max(1, config.plantedReps) : 1;
    std::vector<double> times;
    std::vector<std::uint64_t> candidates;
    int found = 0;

    for (int rep = 1; rep <= reps; ++rep) {
        CompanyParams params;
        params.n = n;
        params.averageDegree = config.averageDegree;
        params.seed = static_cast<std::uint64_t>(rep);
        params.mode = mode;
        Graph company;
        std::string error;
        if (!generateCompany(params, &pattern, company, error)) {
            log << "    erro ao gerar a empresa: " << error << "\n";
            outcome.problem = true;
            return outcome;
        }

        SearchOptions options;
        options.timeLimitSeconds = config.timeoutSeconds;
        SearchContext ctx(options);
        const RunResult result = runSolver(solver, pattern, company, ctx);

        BenchRow row;
        row.solver = solver.id;
        row.series = series;
        row.preset = presetId;
        row.mode = companyModeId(mode);
        row.n = n;
        row.k = pattern.vertexCount();
        row.m = company.edgeCount();
        row.seed = params.seed;
        row.found = result.found;
        row.candidates = result.steps;
        row.timeMs = result.seconds * 1000.0;
        row.timeout = result.timedOut;
        csv << benchCsvLine(row) << "\n";
        csv.flush();

        times.push_back(row.timeMs);
        candidates.push_back(row.candidates);
        if (result.found) ++found;
        if (result.timedOut) outcome.anyTimeout = true;

        if (!result.invalidReason.empty()) {
            log << "    ATENÇÃO: equipe inválida (" << result.invalidReason << ")\n";
            outcome.problem = true;
        }
        if (result.steps == 0) {
            log << "    ATENÇÃO: o solver não testou nenhum candidato (ainda é o stub?). Série interrompida.\n";
            outcome.problem = true;
            return outcome;
        }
    }

    char line[160];
    std::snprintf(line, sizeof line, "    n=%-5d k=%-2d ", n, pattern.vertexCount());
    log << line;
    if (outcome.anyTimeout) {
        log << "tempo limite (" << formatDuration(config.timeoutSeconds) << ")";
    } else {
        log << (mode == CompanyMode::Planted ? std::to_string(found) + "/" + std::to_string(reps) + " encontraram"
                                             : std::string(found > 0 ? "encontrou (!)" : "sem solução"))
            << " · " << formatThousands(median(candidates)) << " candidatos · " << formatDuration(median(times) / 1000.0);
    }
    if (reps > 1) log << " (mediana de " << reps << ")";
    log << "\n";
    log.flush();
    return outcome;
}

}  // namespace

int runBenchmark(const BenchConfig& config, const std::vector<Preset>& presets, std::ostream& csv, std::ostream& log) {
    csv << benchCsvHeader() << "\n";
    int status = 0;
    const CompanyMode modes[] = {CompanyMode::NoSolution, CompanyMode::Planted};

    for (const SolverInfo& solver : allSolvers()) {
        if (!config.onlySolver.empty() && config.onlySolver != solver.id) continue;
        log << "Solver: " << solver.displayName << "\n";
        bool solverBroken = false;

        // Série 1: n crescente, k fixo
        for (const Preset& preset : presets) {
            if (!config.runGrowN || solverBroken) break;
            if (!config.onlyPreset.empty() && config.onlyPreset != preset.id) continue;
            for (CompanyMode mode : modes) {
                if (solverBroken) break;
                log << "\n  [n crescente] " << preset.graph.name << " (k=" << preset.graph.vertexCount() << ") · "
                    << companyModeLabel(mode) << "\n";
                for (int n : kCompanySizes) {
                    if (n < preset.graph.vertexCount() || n > config.maxN) continue;
                    const SizeOutcome outcome = runSize(config, solver, "grow_n", preset.id, preset.graph, n, mode, csv, log);
                    if (outcome.problem) {
                        status = 1;
                        solverBroken = true;
                        break;
                    }
                    if (outcome.anyTimeout) break;
                }
            }
        }

        // Série 2: k crescente, n fixo
        for (CompanyMode mode : modes) {
            if (!config.runGrowK || solverBroken) break;
            log << "\n  [k crescente] moldes aleatórios, n=" << config.growKCompanySize << " · " << companyModeLabel(mode)
                << "\n";
            for (int k = 2; k <= std::min(config.maxK, config.growKCompanySize); ++k) {
                PatternParams params;
                params.k = k;
                params.seed = static_cast<std::uint64_t>(k);
                Graph pattern;
                std::string error;
                if (!generatePattern(params, pattern, error)) {
                    log << "    erro ao gerar o molde: " << error << "\n";
                    status = 1;
                    break;
                }
                const SizeOutcome outcome = runSize(config, solver, "grow_k", "random_k" + std::to_string(k), pattern,
                                                    config.growKCompanySize, mode, csv, log);
                if (outcome.problem) {
                    status = 1;
                    solverBroken = true;
                    break;
                }
                if (outcome.anyTimeout) break;
            }
        }
        log << "\n";
    }
    return status;
}
