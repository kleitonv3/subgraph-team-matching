// Posições dos vértices para desenhar um grafo.

#pragma once

#include <vector>

#include "graph.hpp"
#include "raylib.h"

// Posições normalizadas (0..1) de cada vértice.
// Se todos os vértices têm posição no arquivo, usa essas posições. Senão, calcula um layout de forças
// (Fruchterman–Reingold): vértices se repelem e arestas puxam as pontas, o que espalha o grafo e deixa
// grupos que colaboram próximos. O resultado é determinístico (o mesmo grafo sempre gera o mesmo desenho).
std::vector<Vector2> computeLayout(const Graph& graph);
