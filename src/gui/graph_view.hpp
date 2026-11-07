// Desenho de um grafo (empresa, molde ou miniatura) dentro de uma área da tela.

#pragma once

#include <vector>

#include "graph.hpp"
#include "raylib.h"
#include "theme.hpp"

struct GraphViewStyle {
    float vertexRadius = 16.0f;
    float edgeThickness = 2.0f;
    bool showNames = false;       // nome do funcionário abaixo do vértice
    bool showRoleLabels = false;  // abreviação do papel dentro do vértice
};

// Destaques desenhados por cima do grafo durante a reprodução de uma busca.
// Vetores vazios significam "sem destaque" para aquele item.
struct GraphHighlight {
    std::vector<float> vertexAlpha;  // opacidade de cada vértice (1 = normal)
    std::vector<int> badge;          // número desenhado junto ao vértice (0 = nenhum)
    std::vector<Color> ring;         // anel em volta do vértice (alpha 0 = sem anel)
    float edgeAlpha = 1.0f;          // opacidade das arestas do grafo

    struct Line {
        int a;
        int b;
        Color color;
        float thickness;
        bool dashed;  // tracejada: colaboração exigida que não existe
    };
    std::vector<Line> lines;  // desenhadas por cima das arestas
};

// Raio de vértice adequado para desenhar n vértices numa área do tamanho da área central.
float vertexRadiusFor(int n);

// Converte posições normalizadas (0..1) em coordenadas de tela dentro de `area`, deixando `margin`
// pixels livres nas bordas (para o raio dos vértices e os nomes).
std::vector<Vector2> toScreen(const std::vector<Vector2>& layout, Rectangle area, float margin);

// Desenha arestas, vértices e rótulos. `hovered` é o vértice sob o mouse (-1 = nenhum).
void drawGraph(const Graph& graph, const std::vector<Vector2>& screen, const GraphViewStyle& style,
               const Fonts& fonts, int hovered = -1, const GraphHighlight* highlight = nullptr);

// Vértice sob o ponto (ou -1).
int vertexAt(const std::vector<Vector2>& screen, float radius, Vector2 point);
