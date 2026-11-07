#include "graph.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>

// ---------------------------------------------------------------------------------------------
// Papéis
// ---------------------------------------------------------------------------------------------

namespace {

struct RoleNames {
    Role role;
    const char* keyword;
    const char* displayName;
};

const RoleNames kRoleNames[kRoleCount] = {
    {Role::Leader, "Lider", "Líder"},
    {Role::Dev, "Dev", "Dev"},
    {Role::Designer, "Designer", "Designer"},
    {Role::Architect, "Arquiteto", "Arquiteto"},
    {Role::QA, "QA", "QA"},
    {Role::DevOps, "DevOps", "DevOps"},
    {Role::Data, "Dados", "Dados"},
};

}  // namespace

const char* roleKeyword(Role role) { return kRoleNames[static_cast<int>(role)].keyword; }

const char* roleDisplayName(Role role) { return kRoleNames[static_cast<int>(role)].displayName; }

bool parseRole(const std::string& text, Role& role) {
    for (const RoleNames& names : kRoleNames) {
        if (text == names.keyword || text == names.displayName) {
            role = names.role;
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------------------------
// Construção do grafo
// ---------------------------------------------------------------------------------------------

int Graph::addVertex(const Vertex& vertex) {
    const int index = vertexCount();
    if (index >= stride_) {
        // Dobra a capacidade da matriz e copia as linhas existentes.
        const int newStride = std::max(8, stride_ * 2);
        std::vector<std::uint8_t> bigger(static_cast<std::size_t>(newStride) * newStride, 0);
        for (int row = 0; row < index; ++row) {
            std::copy_n(adjacency_.begin() + static_cast<std::ptrdiff_t>(row) * stride_, index,
                        bigger.begin() + static_cast<std::ptrdiff_t>(row) * newStride);
        }
        adjacency_.swap(bigger);
        stride_ = newStride;
    }
    vertices_.push_back(vertex);
    neighbors_.emplace_back();
    return index;
}

bool Graph::addEdge(int a, int b) {
    if (a == b || hasEdge(a, b)) return false;
    adjacency_[static_cast<std::size_t>(a) * stride_ + b] = 1;
    adjacency_[static_cast<std::size_t>(b) * stride_ + a] = 1;
    neighbors_[a].push_back(b);
    neighbors_[b].push_back(a);
    edges_.emplace_back(std::min(a, b), std::max(a, b));
    return true;
}

void Graph::removeEdge(int a, int b) {
    if (a == b || !hasEdge(a, b)) return;
    adjacency_[static_cast<std::size_t>(a) * stride_ + b] = 0;
    adjacency_[static_cast<std::size_t>(b) * stride_ + a] = 0;
    auto eraseValue = [](std::vector<int>& list, int value) {
        list.erase(std::find(list.begin(), list.end(), value));
    };
    eraseValue(neighbors_[a], b);
    eraseValue(neighbors_[b], a);
    const std::pair<int, int> key(std::min(a, b), std::max(a, b));
    edges_.erase(std::find(edges_.begin(), edges_.end(), key));
}

// ---------------------------------------------------------------------------------------------
// Leitura do formato de texto
// ---------------------------------------------------------------------------------------------

namespace {

std::string trim(const std::string& text) {
    const char* spaces = " \t\r\n";
    const std::size_t first = text.find_first_not_of(spaces);
    if (first == std::string::npos) return "";
    const std::size_t last = text.find_last_not_of(spaces);
    return text.substr(first, last - first + 1);
}

std::vector<std::string> splitTokens(const std::string& text) {
    std::vector<std::string> tokens;
    std::istringstream stream(text);
    std::string token;
    while (stream >> token) tokens.push_back(token);
    return tokens;
}

bool parseInt(const std::string& text, int& value) {
    if (text.empty()) return false;
    errno = 0;
    char* end = nullptr;
    const long parsed = std::strtol(text.c_str(), &end, 10);
    if (errno != 0 || *end != '\0' || parsed < -1000000000L || parsed > 1000000000L) return false;
    value = static_cast<int>(parsed);
    return true;
}

bool parseFloat(const std::string& text, float& value) {
    if (text.empty()) return false;
    errno = 0;
    char* end = nullptr;
    const double parsed = std::strtod(text.c_str(), &end);
    if (errno != 0 || *end != '\0') return false;
    value = static_cast<float>(parsed);
    return true;
}

std::string validRoleList() {
    std::string list;
    for (int i = 0; i < kRoleCount; ++i) {
        if (i > 0) list += ", ";
        list += roleKeyword(static_cast<Role>(i));
    }
    return list;
}

// Nomes não podem ter espaço no arquivo: "_" vira espaço e "-" significa sem nome.
std::string decodeName(const std::string& token) {
    if (token == "-") return "";
    std::string name = token;
    std::replace(name.begin(), name.end(), '_', ' ');
    return name;
}

std::string encodeName(const std::string& name) {
    if (name.empty()) return "-";
    std::string token = name;
    std::replace(token.begin(), token.end(), ' ', '_');
    return token;
}

}  // namespace

bool parseGraph(const std::string& text, const std::string& sourceName, Graph& graph, std::string& error) {
    enum class Section { Header, Vertices, Edges };

    Graph result;
    Section section = Section::Header;
    int declaredVertices = -1;
    int declaredEdges = -1;
    int edgesRead = 0;
    int lineNumber = 0;

    auto fail = [&](const std::string& reason) {
        error = sourceName + ":" + std::to_string(lineNumber) + ": " + reason;
        return false;
    };

    std::istringstream input(text);
    std::string rawLine;
    while (std::getline(input, rawLine)) {
        ++lineNumber;
        if (lineNumber == 1 && rawLine.compare(0, 3, "\xEF\xBB\xBF") == 0) rawLine.erase(0, 3);  // BOM do UTF-8

        const std::size_t comment = rawLine.find('#');
        const std::string line = trim(comment == std::string::npos ? rawLine : rawLine.substr(0, comment));
        if (line.empty()) continue;

        const std::vector<std::string> tokens = splitTokens(line);
        const std::string& keyword = tokens[0];

        // Linhas de vértice: "id papel [nome [x y]]"
        if (section == Section::Vertices && result.vertexCount() < declaredVertices) {
            if (keyword == "edges") {
                return fail("foram declarados " + std::to_string(declaredVertices) + " vértices, mas só " +
                            std::to_string(result.vertexCount()) + " foram listados antes de \"edges\"");
            }
            if (tokens.size() != 2 && tokens.size() != 3 && tokens.size() != 5) {
                return fail("vértice deveria ter o formato \"id papel [nome [x y]]\"");
            }
            int id = 0;
            if (!parseInt(tokens[0], id)) return fail("id de vértice inválido: \"" + tokens[0] + "\"");
            if (id != result.vertexCount()) {
                return fail("esperava o vértice " + std::to_string(result.vertexCount()) + ", mas veio " +
                            std::to_string(id) + " (os ids devem ser 0, 1, 2, ... em ordem)");
            }
            Vertex vertex;
            if (!parseRole(tokens[1], vertex.role)) {
                return fail("papel desconhecido \"" + tokens[1] + "\" (válidos: " + validRoleList() + ")");
            }
            if (tokens.size() >= 3) vertex.name = decodeName(tokens[2]);
            if (tokens.size() == 5) {
                if (!parseFloat(tokens[3], vertex.x) || !parseFloat(tokens[4], vertex.y)) {
                    return fail("posição inválida: \"" + tokens[3] + " " + tokens[4] + "\"");
                }
                if (vertex.x < 0.0f || vertex.x > 1.0f || vertex.y < 0.0f || vertex.y > 1.0f) {
                    return fail("posição fora do intervalo 0..1");
                }
                vertex.hasPosition = true;
            }
            result.addVertex(vertex);
            continue;
        }

        // Linhas de aresta: "a b"
        if (section == Section::Edges && edgesRead < declaredEdges) {
            int a = 0;
            int b = 0;
            if (tokens.size() != 2 || !parseInt(tokens[0], a) || !parseInt(tokens[1], b)) {
                return fail("aresta deveria ter o formato \"a b\"");
            }
            const int n = result.vertexCount();
            if (a < 0 || a >= n || b < 0 || b >= n) {
                return fail("aresta " + tokens[0] + " " + tokens[1] + " usa um vértice inexistente (válidos: 0 a " +
                            std::to_string(n - 1) + ")");
            }
            if (a == b) return fail("aresta de um vértice para ele mesmo (" + tokens[0] + ")");
            if (!result.addEdge(a, b)) return fail("aresta repetida: " + tokens[0] + " " + tokens[1]);
            ++edgesRead;
            continue;
        }

        // Palavras-chave
        if (keyword == "name" || keyword == "desc") {
            if (section != Section::Header) return fail("\"" + keyword + "\" deve vir antes de \"vertices\"");
            const std::string value = trim(line.substr(keyword.size()));
            (keyword == "name" ? result.name : result.description) = value;
        } else if (keyword == "vertices") {
            if (section != Section::Header) return fail("\"vertices\" aparece mais de uma vez");
            if (tokens.size() != 2 || !parseInt(tokens[1], declaredVertices) || declaredVertices < 1) {
                return fail("\"vertices\" deve ser seguido de um número maior que zero");
            }
            section = Section::Vertices;
        } else if (keyword == "edges") {
            if (section == Section::Header) return fail("\"edges\" deve vir depois de \"vertices\"");
            if (section == Section::Edges) return fail("\"edges\" aparece mais de uma vez");
            if (tokens.size() != 2 || !parseInt(tokens[1], declaredEdges) || declaredEdges < 0) {
                return fail("\"edges\" deve ser seguido de um número (0 ou mais)");
            }
            section = Section::Edges;
        } else if (section == Section::Edges) {
            return fail("há mais arestas do que as " + std::to_string(declaredEdges) + " declaradas");
        } else {
            return fail("linha inesperada: \"" + line + "\"");
        }
    }

    if (declaredVertices < 0) return fail("faltou a linha \"vertices N\"");
    if (result.vertexCount() < declaredVertices) {
        return fail("foram declarados " + std::to_string(declaredVertices) + " vértices, mas só " +
                    std::to_string(result.vertexCount()) + " foram listados");
    }
    if (declaredEdges < 0) return fail("faltou a linha \"edges M\"");
    if (edgesRead < declaredEdges) {
        return fail("foram declaradas " + std::to_string(declaredEdges) + " arestas, mas só " +
                    std::to_string(edgesRead) + " foram listadas");
    }

    graph = std::move(result);
    return true;
}

bool loadGraphFile(const std::string& path, Graph& graph, std::string& error) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        error = path + ": não foi possível abrir o arquivo";
        return false;
    }
    std::ostringstream content;
    content << file.rdbuf();
    return parseGraph(content.str(), path, graph, error);
}

void writeGraph(std::ostream& out, const Graph& graph) {
    if (!graph.name.empty()) out << "name " << graph.name << "\n";
    if (!graph.description.empty()) out << "desc " << graph.description << "\n";

    out << "vertices " << graph.vertexCount() << "\n";
    out << std::fixed << std::setprecision(3);
    for (int v = 0; v < graph.vertexCount(); ++v) {
        const Vertex& vertex = graph.vertex(v);
        out << v << " " << roleKeyword(vertex.role) << " " << encodeName(vertex.name);
        if (vertex.hasPosition) out << " " << vertex.x << " " << vertex.y;
        out << "\n";
    }

    out << "edges " << graph.edgeCount() << "\n";
    for (const auto& edge : graph.edges()) out << edge.first << " " << edge.second << "\n";
}
