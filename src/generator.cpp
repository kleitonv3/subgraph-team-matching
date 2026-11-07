#include "generator.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------------------------
// Números aleatórios
// ---------------------------------------------------------------------------------------------

std::uint64_t Random::next() {
    std::uint64_t z = (state_ += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

double Random::uniform() { return static_cast<double>(next() >> 11) * (1.0 / 9007199254740992.0); }

int Random::below(int bound) { return static_cast<int>(next() % static_cast<std::uint64_t>(bound)); }

namespace {

Role sampleRole(Random& random, const RoleWeights& weights) {
    double total = 0.0;
    for (double w : weights) total += w;
    double pick = random.uniform() * total;
    for (int i = 0; i < kRoleCount; ++i) {
        pick -= weights[i];
        if (pick < 0.0) return static_cast<Role>(i);
    }
    return Role::Dev;
}

// Embaralhamento de Fisher-Yates com o nosso gerador (std::shuffle varia entre bibliotecas).
void shuffle(std::vector<int>& values, Random& random) {
    for (int i = static_cast<int>(values.size()) - 1; i > 0; --i) {
        std::swap(values[i], values[random.below(i + 1)]);
    }
}

bool validWeights(const RoleWeights& weights) {
    double total = 0.0;
    for (double w : weights) {
        if (w < 0.0) return false;
        total += w;
    }
    return total > 0.0;
}

}  // namespace

// ---------------------------------------------------------------------------------------------
// Modos
// ---------------------------------------------------------------------------------------------

RoleWeights defaultRoleWeights() {
    RoleWeights weights{};
    weights[static_cast<int>(Role::Leader)] = 0.12;
    weights[static_cast<int>(Role::Dev)] = 0.35;
    weights[static_cast<int>(Role::Designer)] = 0.12;
    weights[static_cast<int>(Role::Architect)] = 0.07;
    weights[static_cast<int>(Role::QA)] = 0.12;
    weights[static_cast<int>(Role::DevOps)] = 0.11;
    weights[static_cast<int>(Role::Data)] = 0.11;
    return weights;
}

const char* companyModeId(CompanyMode mode) {
    switch (mode) {
        case CompanyMode::Random: return "random";
        case CompanyMode::Planted: return "planted";
        case CompanyMode::NoSolution: return "nosolution";
    }
    return "random";
}

bool parseCompanyMode(const std::string& text, CompanyMode& mode) {
    for (CompanyMode candidate : {CompanyMode::Random, CompanyMode::Planted, CompanyMode::NoSolution}) {
        if (text == companyModeId(candidate)) {
            mode = candidate;
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------------------------
// Empresas
// ---------------------------------------------------------------------------------------------

bool generateCompany(const CompanyParams& params, const Graph* pattern, Graph& company, std::string& error) {
    const int n = params.n;
    const int k = pattern ? pattern->vertexCount() : 0;

    if (n < 1) {
        error = "o número de funcionários deve ser maior que zero";
        return false;
    }
    if (params.averageDegree < 0.0) {
        error = "o grau médio não pode ser negativo";
        return false;
    }
    if (!validWeights(params.roles)) {
        error = "os pesos dos papéis devem ser não negativos e somar mais que zero";
        return false;
    }
    if (params.mode != CompanyMode::Random && pattern == nullptr) {
        error = std::string("o modo \"") + companyModeId(params.mode) + "\" precisa de um molde";
        return false;
    }
    if (pattern && k > n) {
        error = "o molde tem " + std::to_string(k) + " vértices, mais do que os " + std::to_string(n) +
                " funcionários da empresa";
        return false;
    }
    if (params.mode == CompanyMode::NoSolution && pattern->edgeCount() == 0) {
        error = "o modo \"nosolution\" precisa de um molde com pelo menos uma aresta";
        return false;
    }

    Random random(params.seed);
    Graph result;
    result.name = std::string("Empresa gerada (n=") + std::to_string(n) + ", " + companyModeId(params.mode) +
                  ", seed=" + std::to_string(params.seed) + ")";

    // 1. Papéis sorteados.
    for (int v = 0; v < n; ++v) {
        Vertex vertex;
        vertex.role = sampleRole(random, params.roles);
        result.addVertex(vertex);
    }

    // 2. Reserva k funcionários com exatamente os papéis do molde. No modo Planted, eles formam a
    //    equipe embutida; nos outros, só garantem que nenhum papel falte.
    std::vector<int> reserved;
    if (pattern) {
        std::vector<int> order(n);
        for (int v = 0; v < n; ++v) order[v] = v;
        shuffle(order, random);
        reserved.assign(order.begin(), order.begin() + k);
        for (int j = 0; j < k; ++j) result.setRole(reserved[j], pattern->role(j));
    }

    // 3. Arestas aleatórias: cada par vira aresta com probabilidade p = d / (n - 1).
    const double p = n > 1 ? std::min(1.0, params.averageDegree / (n - 1)) : 0.0;
    for (int a = 0; a < n; ++a) {
        for (int b = a + 1; b < n; ++b) {
            if (random.uniform() < p) result.addEdge(a, b);
        }
    }

    // 4. Modo Planted: acrescenta as arestas do molde entre os funcionários reservados.
    if (params.mode == CompanyMode::Planted) {
        for (const auto& edge : pattern->edges()) result.addEdge(reserved[edge.first], reserved[edge.second]);
    }

    // 5. Modo NoSolution: escolhe uma aresta {u, v} do molde e remove da empresa TODAS as arestas entre
    //    alguém com o papel de u e alguém com o papel de v. Assim nenhum mapeamento consegue preservar
    //    essa aresta, mas todos os papéis continuam presentes. Entre as arestas do molde, escolhe a que
    //    remove menos arestas da empresa, para alterar o grafo o mínimo possível.
    if (params.mode == CompanyMode::NoSolution) {
        auto samePair = [](Role a, Role b, Role x, Role y) { return (a == x && b == y) || (a == y && b == x); };
        auto countMatching = [&](Role x, Role y) {
            int count = 0;
            for (const auto& edge : result.edges()) {
                if (samePair(result.role(edge.first), result.role(edge.second), x, y)) ++count;
            }
            return count;
        };

        std::pair<Role, Role> blocked(pattern->role(pattern->edges()[0].first),
                                      pattern->role(pattern->edges()[0].second));
        int fewest = std::numeric_limits<int>::max();
        for (const auto& edge : pattern->edges()) {
            const Role x = pattern->role(edge.first);
            const Role y = pattern->role(edge.second);
            const int count = countMatching(x, y);
            if (count < fewest) {
                fewest = count;
                blocked = {x, y};
            }
        }

        std::vector<std::pair<int, int>> toRemove;
        for (const auto& edge : result.edges()) {
            if (samePair(result.role(edge.first), result.role(edge.second), blocked.first, blocked.second)) {
                toRemove.push_back(edge);
            }
        }
        for (const auto& edge : toRemove) result.removeEdge(edge.first, edge.second);

        result.description = std::string("Sem colaborações entre ") + roleDisplayName(blocked.first) + " e " +
                             roleDisplayName(blocked.second);
    }

    company = std::move(result);
    return true;
}

// ---------------------------------------------------------------------------------------------
// Moldes
// ---------------------------------------------------------------------------------------------

bool generatePattern(const PatternParams& params, Graph& pattern, std::string& error) {
    const int k = params.k;
    if (k < 1) {
        error = "o molde deve ter pelo menos 1 vértice";
        return false;
    }
    if (params.extraEdgeProbability < 0.0 || params.extraEdgeProbability > 1.0) {
        error = "a probabilidade de arestas extras deve estar entre 0 e 1";
        return false;
    }
    if (!validWeights(params.roles)) {
        error = "os pesos dos papéis devem ser não negativos e somar mais que zero";
        return false;
    }

    Random random(params.seed);
    Graph result;
    result.name = "Molde aleatório (k=" + std::to_string(k) + ", seed=" + std::to_string(params.seed) + ")";

    const double pi = 3.14159265358979323846;
    for (int j = 0; j < k; ++j) {
        Vertex vertex;
        vertex.role = sampleRole(random, params.roles);
        const double angle = -pi / 2 + 2 * pi * j / k;
        vertex.x = static_cast<float>(0.5 + 0.4 * std::cos(angle));
        vertex.y = static_cast<float>(0.5 + 0.4 * std::sin(angle));
        vertex.hasPosition = true;
        result.addVertex(vertex);
    }

    // Árvore aleatória (garante que o molde é conexo) + arestas extras.
    for (int j = 1; j < k; ++j) result.addEdge(j, random.below(j));
    for (int a = 0; a < k; ++a) {
        for (int b = a + 1; b < k; ++b) {
            if (!result.hasEdge(a, b) && random.uniform() < params.extraEdgeProbability) result.addEdge(a, b);
        }
    }

    pattern = std::move(result);
    return true;
}
