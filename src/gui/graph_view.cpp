#include "graph_view.hpp"

#include <algorithm>
#include <cmath>

float vertexRadiusFor(int n) {
    if (n <= 0) return 16.0f;
    return std::clamp(75.0f / std::sqrt(static_cast<float>(n)), 5.0f, 20.0f);
}

std::vector<Vector2> toScreen(const std::vector<Vector2>& layout, Rectangle area, float margin) {
    const float width = std::max(area.width - 2 * margin, 1.0f);
    const float height = std::max(area.height - 2 * margin, 1.0f);
    std::vector<Vector2> screen(layout.size());
    for (std::size_t v = 0; v < layout.size(); ++v) {
        screen[v] = {area.x + margin + layout[v].x * width, area.y + margin + layout[v].y * height};
    }
    return screen;
}

void drawGraph(const Graph& graph, const std::vector<Vector2>& screen, const GraphViewStyle& style,
               const Fonts& fonts, int hovered) {
    // Arestas primeiro, para ficarem por baixo dos vértices
    for (const auto& edge : graph.edges()) {
        const bool touchesHovered = edge.first == hovered || edge.second == hovered;
        DrawLineEx(screen[edge.first], screen[edge.second], style.edgeThickness * (touchesHovered ? 1.75f : 1.0f),
                   touchesHovered ? theme::kTextSecondary : theme::kEdge);
    }

    const float outline = std::max(1.5f, style.vertexRadius * 0.12f);
    for (int v = 0; v < graph.vertexCount(); ++v) {
        const Vector2 center = screen[v];
        const Color fill = theme::roleColor(graph.role(v));

        if (v == hovered) DrawCircleV(center, style.vertexRadius + outline + 3, Fade(theme::kAccent, 0.35f));
        DrawCircleV(center, style.vertexRadius + outline, theme::kVertexOutline);
        DrawCircleV(center, style.vertexRadius, fill);

        if (style.showRoleLabels) {
            const char* label = theme::roleShortLabel(graph.role(v));
            const Font& font = style.vertexRadius >= 18 ? fonts.bodyBold : fonts.small;
            const float w = measureText(font, label);
            drawText(font, label, center.x - w / 2, center.y - font.baseSize / 2.0f, WHITE);
        }

        if (style.showNames && !graph.vertex(v).name.empty()) {
            const char* name = graph.vertex(v).name.c_str();
            const float w = measureText(fonts.small, name);
            drawText(fonts.small, name, center.x - w / 2, center.y + style.vertexRadius + outline + 3,
                     theme::kTextPrimary);
        }
    }
}

int vertexAt(const std::vector<Vector2>& screen, float radius, Vector2 point) {
    // Procura do último para o primeiro: o vértice desenhado por cima tem prioridade
    for (int v = static_cast<int>(screen.size()) - 1; v >= 0; --v) {
        if (CheckCollisionPointCircle(point, screen[v], radius + 2)) return v;
    }
    return -1;
}
