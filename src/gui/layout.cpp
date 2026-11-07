#include "layout.hpp"

#include <algorithm>
#include <cmath>

#include "generator.hpp"

namespace {

bool allHavePositions(const Graph& graph) {
    for (int v = 0; v < graph.vertexCount(); ++v) {
        if (!graph.vertex(v).hasPosition) return false;
    }
    return graph.vertexCount() > 0;
}

// Reescala as posições para ocupar todo o intervalo 0..1, mantendo a proporção.
void normalize(std::vector<Vector2>& positions) {
    if (positions.empty()) return;
    float minX = positions[0].x, maxX = positions[0].x, minY = positions[0].y, maxY = positions[0].y;
    for (const Vector2& p : positions) {
        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
    }
    const float span = std::max({maxX - minX, maxY - minY, 1e-6f});
    const float offsetX = (span - (maxX - minX)) / 2;
    const float offsetY = (span - (maxY - minY)) / 2;
    for (Vector2& p : positions) {
        p.x = (p.x - minX + offsetX) / span;
        p.y = (p.y - minY + offsetY) / span;
    }
}

}  // namespace

std::vector<Vector2> computeLayout(const Graph& graph) {
    const int n = graph.vertexCount();
    std::vector<Vector2> positions(n);

    if (allHavePositions(graph)) {
        for (int v = 0; v < n; ++v) positions[v] = {graph.vertex(v).x, graph.vertex(v).y};
        return positions;
    }
    if (n == 1) {
        positions[0] = {0.5f, 0.5f};
        return positions;
    }

    // Posição inicial: círculo com uma pequena perturbação (seed fixa: resultado determinístico).
    Random random(42);
    const float pi = 3.14159265f;
    for (int v = 0; v < n; ++v) {
        const float angle = 2 * pi * v / n;
        positions[v] = {0.5f + 0.4f * std::cos(angle) + 0.02f * static_cast<float>(random.uniform() - 0.5),
                        0.5f + 0.4f * std::sin(angle) + 0.02f * static_cast<float>(random.uniform() - 0.5)};
    }

    const float k = std::sqrt(1.0f / n);  // distância ideal entre vértices
    const int iterations = 300;
    const float gravity = 0.08f;          // puxa para o centro (mantém vértices isolados por perto)
    std::vector<Vector2> displacement(n);

    for (int it = 0; it < iterations; ++it) {
        const float temperature = 0.1f * (1.0f - static_cast<float>(it) / iterations) + 0.002f;
        std::fill(displacement.begin(), displacement.end(), Vector2{0, 0});

        // Repulsão entre todos os pares
        for (int a = 0; a < n; ++a) {
            for (int b = a + 1; b < n; ++b) {
                const float dx = positions[a].x - positions[b].x;
                const float dy = positions[a].y - positions[b].y;
                const float dist = std::max(std::sqrt(dx * dx + dy * dy), 0.01f);
                const float force = k * k / dist;
                displacement[a].x += dx / dist * force;
                displacement[a].y += dy / dist * force;
                displacement[b].x -= dx / dist * force;
                displacement[b].y -= dy / dist * force;
            }
        }

        // Atração ao longo das arestas
        for (const auto& edge : graph.edges()) {
            const int a = edge.first;
            const int b = edge.second;
            const float dx = positions[a].x - positions[b].x;
            const float dy = positions[a].y - positions[b].y;
            const float dist = std::max(std::sqrt(dx * dx + dy * dy), 0.01f);
            const float force = dist * dist / k;
            displacement[a].x -= dx / dist * force;
            displacement[a].y -= dy / dist * force;
            displacement[b].x += dx / dist * force;
            displacement[b].y += dy / dist * force;
        }

        // Gravidade e movimento limitado pela temperatura
        for (int v = 0; v < n; ++v) {
            displacement[v].x += (0.5f - positions[v].x) * gravity * n * k;
            displacement[v].y += (0.5f - positions[v].y) * gravity * n * k;
            const float length =
                std::max(std::sqrt(displacement[v].x * displacement[v].x + displacement[v].y * displacement[v].y), 1e-6f);
            const float step = std::min(length, temperature);
            positions[v].x += displacement[v].x / length * step;
            positions[v].y += displacement[v].y / length * step;
        }
    }

    normalize(positions);
    return positions;
}
