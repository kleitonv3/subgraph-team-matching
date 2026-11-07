// teams_bench: interface de linha de comando para listar os dados, rodar um solver, gerar instâncias e
// rodar as séries do benchmark (--bench), que gravam results/bench.csv.

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "benchmark.hpp"
#include "dataset.hpp"
#include "format.hpp"
#include "generator.hpp"
#include "graph.hpp"
#include "solver.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#endif


namespace {

const char* kUsage = R"(Uso: teams_bench <comando> [opções]

Comandos:
  --list                     lista a empresa, os presets e os solvers
  --run PRESET               roda um solver num preset
  --generate-company N       escreve na saída uma empresa gerada com N funcionários
  --generate-pattern K       escreve na saída um molde aleatório com K vértices
  --bench                    roda as séries do benchmark e grava o CSV (veja abaixo)
  --help                     mostra esta ajuda

Opções:
  --data PASTA               pasta de dados (padrão: data)
  --company ARQUIVO          empresa usada pelo --run (padrão: <data>/company.txt)
  --n N                      no --run, gera uma empresa com N funcionários em vez de usar o arquivo
  --mode MODO                empresa gerada: random, planted ou nosolution (padrão: planted)
  --pattern PRESET           molde usado pelo --generate-company nos modos planted e nosolution
  --degree D                 grau médio da empresa gerada (padrão: 6)
  --seed S                   seed da geração (padrão: 1)
  --solver ID                solver do --run (padrão: brute_force)
  --timeout SEG              tempo limite do --run em segundos (padrão: 10; 0 = sem limite)
  --all                      no --run, continua depois da primeira equipe e conta todas

Opções do --bench:
  --out ARQUIVO              onde gravar o CSV (padrão: results/bench.csv)
  --timeout SEG              tempo limite de cada execução (padrão: 10)
  --series S                 n, k ou all (padrão: all)
  --only PRESET              só um preset na série n crescente
  --solver ID                só um solver (padrão: todos os registrados)
  --reps R                   repetições no modo plantada, com seeds 1..R (padrão: 3)
  --max-n N                  maior empresa da série n crescente (padrão: 4096)
  --k-n N                    tamanho fixo da empresa na série k crescente (padrão: 20)
  --degree D                 grau médio das empresas geradas (padrão: 6)

PRESET pode ser o nome de um arquivo em <data>/presets (sem .txt) ou o caminho de um arquivo.

Exemplos:
  teams_bench --list
  teams_bench --run agile_squad
  teams_bench --run star_team --n 200 --mode nosolution --timeout 30
  teams_bench --generate-company 50 --pattern agile_squad --mode planted --seed 7
  teams_bench --bench --timeout 60
)";

struct Options {
    std::string command;
    std::string commandArg;
    std::string dataDir = "data";
    std::string companyFile;
    std::string pattern;
    std::string solver = "brute_force";
    bool solverGiven = false;
    CompanyMode mode = CompanyMode::Planted;
    int n = 0;
    double degree = 6.0;
    std::uint64_t seed = 1;
    double timeout = 10.0;
    bool findAll = false;

    // --bench
    std::string out = "results/bench.csv";
    std::string series = "all";
    std::string only;
    int reps = 3;
    int maxN = 4096;
    int growKSize = 20;
};

// --- Formatação ------------------------------------------------------------------------------

// Número de caracteres visíveis de um texto UTF-8 (para alinhar colunas com acentos).
int visibleLength(const std::string& text) {
    int count = 0;
    for (unsigned char c : text) {
        if ((c & 0xC0) != 0x80) ++count;
    }
    return count;
}

std::string padRight(const std::string& text, int width) {
    const int missing = width - visibleLength(text);
    return missing > 0 ? text + std::string(missing, ' ') : text;
}

std::string personLabel(const Graph& company, int v) {
    const std::string& name = company.vertex(v).name;
    return (name.empty() ? "Funcionário" : name) + " (#" + std::to_string(v) + ")";
}

std::string roleSummary(const Graph& graph) {
    int counts[kRoleCount] = {};
    for (int v = 0; v < graph.vertexCount(); ++v) ++counts[static_cast<int>(graph.role(v))];
    std::string summary;
    for (int r = 0; r < kRoleCount; ++r) {
        if (counts[r] == 0) continue;
        if (!summary.empty()) summary += ", ";
        summary += roleDisplayName(static_cast<Role>(r));
        if (counts[r] > 1) summary += " ×" + std::to_string(counts[r]);
    }
    return summary;
}

// --- Leitura dos argumentos ------------------------------------------------------------------

bool parseArgs(int argc, char** argv, Options& options, std::string& error) {
    auto needValue = [&](int& i, std::string& value) {
        if (i + 1 >= argc) {
            error = std::string("faltou o valor de ") + argv[i];
            return false;
        }
        value = argv[++i];
        return true;
    };
    auto toInt = [&](const std::string& text, int& value) {
        char* end = nullptr;
        const long parsed = std::strtol(text.c_str(), &end, 10);
        if (text.empty() || *end != '\0' || parsed < 0 || parsed > 100000000L) {
            error = "número inválido: \"" + text + "\"";
            return false;
        }
        value = static_cast<int>(parsed);
        return true;
    };
    auto toDouble = [&](const std::string& text, double& value) {
        char* end = nullptr;
        value = std::strtod(text.c_str(), &end);
        if (text.empty() || *end != '\0' || value < 0) {
            error = "número inválido: \"" + text + "\"";
            return false;
        }
        return true;
    };

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        std::string value;
        if (arg == "--list" || arg == "--help" || arg == "-h" || arg == "--bench") {
            options.command = arg == "-h" ? "--help" : arg;
        } else if (arg == "--run" || arg == "--generate-company" || arg == "--generate-pattern") {
            options.command = arg;
            if (!needValue(i, options.commandArg)) return false;
        } else if (arg == "--data") {
            if (!needValue(i, options.dataDir)) return false;
        } else if (arg == "--company") {
            if (!needValue(i, options.companyFile)) return false;
        } else if (arg == "--pattern") {
            if (!needValue(i, options.pattern)) return false;
        } else if (arg == "--solver") {
            if (!needValue(i, options.solver)) return false;
            options.solverGiven = true;
        } else if (arg == "--out") {
            if (!needValue(i, options.out)) return false;
        } else if (arg == "--series") {
            if (!needValue(i, options.series)) return false;
            if (options.series != "n" && options.series != "k" && options.series != "all") {
                error = "série inválida: \"" + options.series + "\" (válidas: n, k, all)";
                return false;
            }
        } else if (arg == "--only") {
            if (!needValue(i, options.only)) return false;
        } else if (arg == "--reps") {
            if (!needValue(i, value) || !toInt(value, options.reps)) return false;
        } else if (arg == "--max-n") {
            if (!needValue(i, value) || !toInt(value, options.maxN)) return false;
        } else if (arg == "--k-n") {
            if (!needValue(i, value) || !toInt(value, options.growKSize)) return false;
        } else if (arg == "--n") {
            if (!needValue(i, value) || !toInt(value, options.n)) return false;
        } else if (arg == "--mode") {
            if (!needValue(i, value)) return false;
            if (!parseCompanyMode(value, options.mode)) {
                error = "modo inválido: \"" + value + "\" (válidos: random, planted, nosolution)";
                return false;
            }
        } else if (arg == "--degree") {
            if (!needValue(i, value) || !toDouble(value, options.degree)) return false;
        } else if (arg == "--seed") {
            int seed = 0;
            if (!needValue(i, value) || !toInt(value, seed)) return false;
            options.seed = static_cast<std::uint64_t>(seed);
        } else if (arg == "--timeout") {
            if (!needValue(i, value) || !toDouble(value, options.timeout)) return false;
        } else if (arg == "--all") {
            options.findAll = true;
        } else {
            error = "opção desconhecida: \"" + arg + "\"";
            return false;
        }
    }
    if (options.command.empty()) options.command = "--help";
    if (options.companyFile.empty()) options.companyFile = options.dataDir + "/company.txt";
    return true;
}

// --- Dados -----------------------------------------------------------------------------------

std::string presetPath(const Options& options, const std::string& preset) {
    if (preset.find('/') != std::string::npos || preset.find('\\') != std::string::npos ||
        (preset.size() > 4 && preset.compare(preset.size() - 4, 4, ".txt") == 0)) {
        return preset;
    }
    return options.dataDir + "/presets/" + preset + ".txt";
}

// --- Comandos --------------------------------------------------------------------------------

int commandList(const Options& options) {
    Graph company;
    std::string error;
    if (!loadGraphFile(options.companyFile, company, error)) {
        std::cerr << "Erro: " << error << "\n";
        return 1;
    }
    std::cout << "Empresa: " << options.companyFile << "\n";
    std::cout << "  " << company.name << ": " << company.vertexCount() << " funcionários, " << company.edgeCount()
              << " colaborações\n";
    std::cout << "  Papéis: " << roleSummary(company) << "\n\n";

    std::vector<Preset> presets;
    if (!loadPresets(options.dataDir, presets, error)) {
        std::cerr << "Erro: " << error << "\n";
        return 1;
    }
    std::cout << "Presets (" << options.dataDir << "/presets):\n";
    for (const Preset& preset : presets) {
        std::cout << "  " << padRight(preset.id, 20) << padRight(preset.graph.name, 22) << "k="
                  << preset.graph.vertexCount() << "  arestas=" << padRight(std::to_string(preset.graph.edgeCount()), 3)
                  << "  " << roleSummary(preset.graph) << "\n";
    }

    std::cout << "\nSolvers:\n";
    for (const SolverInfo& solver : allSolvers()) {
        std::cout << "  " << padRight(solver.id, 20) << solver.displayName << "\n";
    }
    return 0;
}

int commandRun(const Options& options) {
    std::string error;

    Graph pattern;
    if (!loadGraphFile(presetPath(options, options.commandArg), pattern, error)) {
        std::cerr << "Erro: " << error << "\n";
        return 1;
    }

    Graph company;
    if (options.n > 0) {
        CompanyParams params;
        params.n = options.n;
        params.averageDegree = options.degree;
        params.seed = options.seed;
        params.mode = options.mode;
        if (!generateCompany(params, &pattern, company, error)) {
            std::cerr << "Erro: " << error << "\n";
            return 1;
        }
    } else if (!loadGraphFile(options.companyFile, company, error)) {
        std::cerr << "Erro: " << error << "\n";
        return 1;
    }

    const SolverInfo* solver = findSolver(options.solver);
    if (!solver) {
        std::cerr << "Erro: solver desconhecido \"" << options.solver << "\" (veja --list)\n";
        return 1;
    }

    SearchOptions searchOptions;
    searchOptions.timeLimitSeconds = options.timeout;
    searchOptions.findAll = options.findAll;
    SearchContext ctx(searchOptions);

    std::cout << "Molde:    " << pattern.name << " (k=" << pattern.vertexCount() << ", " << pattern.edgeCount()
              << " arestas)\n";
    std::cout << "Empresa:  " << company.name << " (n=" << company.vertexCount() << ", " << company.edgeCount()
              << " arestas)\n";
    if (!company.description.empty()) std::cout << "          " << company.description << "\n";
    std::cout << "Solver:   " << solver->displayName << "\n\n";

    const RunResult result = runSolver(*solver, pattern, company, ctx);

    if (result.found) {
        std::cout << "Resultado: equipe encontrada\n";
        for (int j = 0; j < pattern.vertexCount(); ++j) {
            const int person = j < static_cast<int>(result.mapping.size()) ? result.mapping[j] : -1;
            std::cout << "  " << padRight(roleDisplayName(pattern.role(j)), 10) << " -> "
                      << (person >= 0 && person < company.vertexCount() ? personLabel(company, person) : "?") << "\n";
        }
    } else if (result.timedOut) {
        std::cout << "Resultado: tempo limite estourado (" << formatDuration(options.timeout) << ") sem encontrar equipe\n";
    } else {
        std::cout << "Resultado: sem solução\n";
    }

    std::cout << "\nCandidatos testados: " << formatThousands(result.steps) << "\n";
    if (options.findAll) std::cout << "Equipes encontradas: " << formatThousands(result.matches) << "\n";
    if (result.found && result.timedOut) std::cout << "A busca foi interrompida pelo tempo limite.\n";
    std::cout << "Tempo: " << formatDuration(result.seconds) << "\n";

    if (!result.invalidReason.empty()) {
        std::cout << "\nATENÇÃO: o solver devolveu uma equipe inválida: " << result.invalidReason << "\n";
        return 2;
    }
    return 0;
}

int commandGenerateCompany(const Options& options) {
    std::string error;
    CompanyParams params;
    if (options.commandArg.empty() || std::strtol(options.commandArg.c_str(), nullptr, 10) <= 0) {
        std::cerr << "Erro: --generate-company precisa de um número de funcionários maior que zero\n";
        return 1;
    }
    params.n = static_cast<int>(std::strtol(options.commandArg.c_str(), nullptr, 10));
    params.averageDegree = options.degree;
    params.seed = options.seed;
    params.mode = options.pattern.empty() ? CompanyMode::Random : options.mode;

    Graph pattern;
    if (!options.pattern.empty() && !loadGraphFile(presetPath(options, options.pattern), pattern, error)) {
        std::cerr << "Erro: " << error << "\n";
        return 1;
    }
    Graph company;
    if (!generateCompany(params, options.pattern.empty() ? nullptr : &pattern, company, error)) {
        std::cerr << "Erro: " << error << "\n";
        return 1;
    }
    writeGraph(std::cout, company);
    return 0;
}

int commandGeneratePattern(const Options& options) {
    std::string error;
    PatternParams params;
    params.k = static_cast<int>(std::strtol(options.commandArg.c_str(), nullptr, 10));
    params.seed = options.seed;
    Graph pattern;
    if (!generatePattern(params, pattern, error)) {
        std::cerr << "Erro: " << error << "\n";
        return 1;
    }
    writeGraph(std::cout, pattern);
    return 0;
}

int commandBench(const Options& options) {
    std::string error;
    std::vector<Preset> presets;
    if (!loadPresets(options.dataDir, presets, error)) {
        std::cerr << "Erro: " << error << "\n";
        return 1;
    }
    if (!options.only.empty() &&
        std::none_of(presets.begin(), presets.end(), [&](const Preset& p) { return p.id == options.only; })) {
        std::cerr << "Erro: preset desconhecido \"" << options.only << "\" (veja --list)\n";
        return 1;
    }
    if (options.solverGiven && !findSolver(options.solver)) {
        std::cerr << "Erro: solver desconhecido \"" << options.solver << "\" (veja --list)\n";
        return 1;
    }

    // Cria a pasta do arquivo de saída, se precisar
    const std::filesystem::path outPath = std::filesystem::u8path(options.out);
    std::error_code ec;
    if (outPath.has_parent_path()) std::filesystem::create_directories(outPath.parent_path(), ec);
    std::ofstream csv(outPath, std::ios::binary);
    if (!csv) {
        std::cerr << "Erro: não foi possível criar " << options.out << "\n";
        return 1;
    }

    BenchConfig config;
    config.timeoutSeconds = options.timeout;
    config.plantedReps = options.reps;
    config.averageDegree = options.degree;
    config.maxN = options.maxN;
    config.growKCompanySize = options.growKSize;
    config.runGrowN = options.series != "k";
    config.runGrowK = options.series != "n";
    config.onlyPreset = options.only;
    if (options.solverGiven) config.onlySolver = options.solver;

    std::cout << "Benchmark · tempo limite " << formatDuration(options.timeout) << " por execução · grau médio "
              << options.degree << "\n";
    std::cout << "Resultados em " << options.out << "\n\n";
    const int status = runBenchmark(config, presets, csv, std::cout);
    std::cout << "Pronto. Abra a aba Benchmark da interface para ver os gráficos.\n";
    return status;
}

}  // namespace

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);     // acentos corretos no terminal do Windows
    _setmode(_fileno(stdout), _O_BINARY);  // quebras de linha LF, iguais às do Linux
#endif

    Options options;
    std::string error;
    if (!parseArgs(argc, argv, options, error)) {
        std::cerr << "Erro: " << error << "\n\n" << kUsage;
        return 1;
    }

    if (options.command == "--list") return commandList(options);
    if (options.command == "--run") return commandRun(options);
    if (options.command == "--generate-company") return commandGenerateCompany(options);
    if (options.command == "--generate-pattern") return commandGeneratePattern(options);
    if (options.command == "--bench") return commandBench(options);
    std::cout << kUsage;
    return 0;
}
