// Grafo usado tanto para a empresa (grafo social) quanto para o molde da equipe.
//
// Cada vértice é uma pessoa (na empresa) ou uma posição do time (no molde), com um papel. As arestas
// são não direcionadas: {a, b} significa que a e b têm histórico de boa colaboração (na empresa) ou
// que essa colaboração é exigida (no molde).

#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>
#include <utility>
#include <vector>

// Papéis possíveis de um funcionário. A comparação é sempre por igualdade exata.
enum class Role : std::uint8_t { Leader, Dev, Designer, Architect, QA, DevOps, Data };

constexpr int kRoleCount = 7;

// Palavra usada nos arquivos de dados (ex.: "Lider").
const char* roleKeyword(Role role);
// Nome para mostrar na interface, com acento (ex.: "Líder").
const char* roleDisplayName(Role role);
// Converte a palavra do arquivo em papel. Aceita também a forma acentuada ("Líder").
// Retorna false se a palavra não for um papel conhecido.
bool parseRole(const std::string& text, Role& role);

struct Vertex {
    Role role = Role::Dev;
    std::string name;          // pode ser vazio
    float x = 0.0f;            // posição normalizada em 0..1, usada pela interface
    float y = 0.0f;
    bool hasPosition = false;  // false: a interface calcula a posição automaticamente
};

class Graph {
public:
    std::string name;
    std::string description;

    // Número de vértices (n na empresa, k no molde).
    int vertexCount() const { return static_cast<int>(vertices_.size()); }
    // Número de arestas.
    int edgeCount() const { return static_cast<int>(edges_.size()); }

    const Vertex& vertex(int v) const { return vertices_[v]; }
    Role role(int v) const { return vertices_[v].role; }

    // true se existe a aresta {a, b}. Custo O(1) (matriz de adjacência).
    bool hasEdge(int a, int b) const { return adjacency_[static_cast<std::size_t>(a) * stride_ + b] != 0; }

    // Vizinhos de v, em ordem crescente de inserção.
    const std::vector<int>& neighbors(int v) const { return neighbors_[v]; }
    int degree(int v) const { return static_cast<int>(neighbors_[v].size()); }

    // Lista de arestas {a, b}, cada uma uma única vez.
    const std::vector<std::pair<int, int>>& edges() const { return edges_; }

    // --- Construção -------------------------------------------------------------------------

    // Adiciona um vértice e retorna o seu índice (0, 1, 2, ...).
    int addVertex(const Vertex& vertex);
    // Adiciona a aresta {a, b}. Retorna false (sem alterar nada) se a == b ou se a aresta já existe.
    bool addEdge(int a, int b);
    // Remove a aresta {a, b}, se existir.
    void removeEdge(int a, int b);
    // Troca o papel de um vértice.
    void setRole(int v, Role role) { vertices_[v].role = role; }

private:
    std::vector<Vertex> vertices_;
    std::vector<std::vector<int>> neighbors_;
    std::vector<std::pair<int, int>> edges_;
    // Matriz de adjacência linha a linha, com `stride_` colunas. A capacidade cresce dobrando, para
    // que adicionar n vértices custe O(n²) no total, e não O(n³).
    std::vector<std::uint8_t> adjacency_;
    int stride_ = 0;
};

// Lê um grafo no formato de texto descrito no SPEC (seção 3).
// Em caso de erro, retorna false e preenche `error` com "arquivo:linha: motivo".
bool loadGraphFile(const std::string& path, Graph& graph, std::string& error);

// Mesmo formato, a partir de um texto já carregado. `sourceName` aparece nas mensagens de erro.
bool parseGraph(const std::string& text, const std::string& sourceName, Graph& graph, std::string& error);

// Escreve o grafo no mesmo formato lido por loadGraphFile.
void writeGraph(std::ostream& out, const Graph& graph);
