// teams_bench: interface de linha de comando para listar os dados, rodar um solver e gerar instâncias.
// As séries completas do benchmark (CSV) entram na Fase 6.

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

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

namespace fs = std::filesystem;

namespace {

const char* kUsage = R"(Uso: teams_bench <comando> [opções]

Comandos:
  --list                     lista a empresa, os presets e os solvers
  --run PRESET               roda um solver num preset
  --generate-company N       escreve na saída uma empresa gerada com N funcionários
  --generate-pattern K       escreve na saída um molde aleatório com K vértices
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

PRESET pode ser o nome de um arquivo em <data>/presets (sem .txt) ou o caminho de um arquivo.

Exemplos:
  teams_bench --list
  teams_bench --run agile_squad
  teams_bench --run star_team --n 200 --mode nosolution --timeout 30
  teams_bench --generate-company 50 --pattern agile_squad --mode planted --seed 7
)";

struct Options {
    std::string command;
    std::string commandArg;
    std::string dataDir = "data";
    std::string companyFile;
    std::string pattern;
    std::string solver = "brute_force";
    CompanyMode mode = CompanyMode::Planted;
    int n = 0;
    double degree = 6.0;
    std::uint64_t seed = 1;
    double timeout = 10.0;
    bool findAll = false;
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

// 1234567 -> "1.234.567"
std::string withThousands(std::uint64_t value) {
    std::string digits = std::to_string(value);
    std::string result;
    const int length = static_cast<int>(digits.size());
    for (int i = 0; i < length; ++i) {
        if (i > 0 && (length - i) % 3 == 0) result += '.';
        result += digits[i];
    }
    return result;
}

std::string formatSeconds(double seconds) {
    char buffer[64];
    if (seconds < 1.0) {
        std::snprintf(buffer, sizeof buffer, "%.3f ms", seconds * 1000.0);
    } else {
        std::snprintf(buffer, sizeof buffer, "%.3f s", seconds);
    }
    std::string text = buffer;
    std::replace(text.begin(), text.end(), '.', ',');
    return text;
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
        if (arg == "--list" || arg == "--help" || arg == "-h") {
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

struct PresetEntry {
    std::string id;
    Graph graph;
};

// Lê todos os presets da pasta, em ordem de dificuldade (k, depois número de arestas).
bool loadPresets(const Options& options, std::vector<PresetEntry>& presets, std::string& error) {
    const fs::path dir = fs::u8path(options.dataDir + "/presets");
    std::error_code ec;
    if (!fs::is_directory(dir, ec)) {
        error = options.dataDir + "/presets: pasta não encontrada";
        return false;
    }
    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".txt") continue;
        PresetEntry preset;
        preset.id = entry.path().stem().u8string();
        if (!loadGraphFile(entry.path().u8string(), preset.graph, error)) return false;
        presets.push_back(std::move(preset));
    }
    std::sort(presets.begin(), presets.end(), [](const PresetEntry& a, const PresetEntry& b) {
        if (a.graph.vertexCount() != b.graph.vertexCount()) return a.graph.vertexCount() < b.graph.vertexCount();
        if (a.graph.edgeCount() != b.graph.edgeCount()) return a.graph.edgeCount() < b.graph.edgeCount();
        return a.id < b.id;
    });
    return true;
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

    std::vector<PresetEntry> presets;
    if (!loadPresets(options, presets, error)) {
        std::cerr << "Erro: " << error << "\n";
        return 1;
    }
    std::cout << "Presets (" << options.dataDir << "/presets):\n";
    for (const PresetEntry& preset : presets) {
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
        std::cout << "Resultado: tempo limite estourado (" << formatSeconds(options.timeout) << ") sem encontrar equipe\n";
    } else {
        std::cout << "Resultado: sem solução\n";
    }

    std::cout << "\nCandidatos testados: " << withThousands(result.steps) << "\n";
    if (options.findAll) std::cout << "Equipes encontradas: " << withThousands(result.matches) << "\n";
    if (result.found && result.timedOut) std::cout << "A busca foi interrompida pelo tempo limite.\n";
    std::cout << "Tempo: " << formatSeconds(result.seconds) << "\n";

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
    std::cout << kUsage;
    return 0;
}
