#include "graph_view.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace {

void drawDashedLine(Vector2 from, Vector2 to, float thickness, Color color) {
    const float dx = to.x - from.x;
    const float dy = to.y - from.y;
    const float length = std::sqrt(dx * dx + dy * dy);
    if (length < 1.0f) return;
    const float dash = 9.0f;
    const float gap = 6.0f;
    for (float t = 0; t < length; t += dash + gap) {
        const float end = std::min(t + dash, length);
        DrawLineEx({from.x + dx * t / length, from.y + dy * t / length},
                   {from.x + dx * end / length, from.y + dy * end / length}, thickness, color);
    }
}

template <typename T>
T valueOr(const std::vector<T>& values, int index, T fallback) {
    return index < static_cast<int>(values.size()) ? values[index] : fallback;
}

}  // namespace

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
               const Fonts& fonts, int hovered, const GraphHighlight* highlight) {
    static const GraphHighlight kNone;
    const GraphHighlight& hl = highlight ? *highlight : kNone;

    // Arestas primeiro, para ficarem por baixo dos vértices
    for (const auto& edge : graph.edges()) {
        const bool touchesHovered = edge.first == hovered || edge.second == hovered;
        const Color color = touchesHovered ? theme::kTextSecondary : theme::kEdge;
        DrawLineEx(screen[edge.first], screen[edge.second], style.edgeThickness * (touchesHovered ? 1.75f : 1.0f),
                   Fade(color, hl.edgeAlpha));
    }
    for (const GraphHighlight::Line& line : hl.lines) {
        if (line.dashed) {
            drawDashedLine(screen[line.a], screen[line.b], line.thickness, line.color);
        } else {
            DrawLineEx(screen[line.a], screen[line.b], line.thickness, line.color);
        }
    }

    const float outline = std::max(1.5f, style.vertexRadius * 0.12f);
    for (int v = 0; v < graph.vertexCount(); ++v) {
        const Vector2 center = screen[v];
        const float alpha = valueOr(hl.vertexAlpha, v, 1.0f);
        const Color fill = Fade(theme::roleColor(graph.role(v)), alpha);
        const Color ring = valueOr(hl.ring, v, Color{0, 0, 0, 0});

        if (ring.a > 0) DrawCircleV(center, style.vertexRadius + outline + 4, ring);
        if (v == hovered) DrawCircleV(center, style.vertexRadius + outline + 3, Fade(theme::kAccent, 0.35f));
        DrawCircleV(center, style.vertexRadius + outline, Fade(theme::kVertexOutline, std::max(alpha, 0.6f)));
        DrawCircleV(center, style.vertexRadius, fill);

        if (style.showRoleLabels) {
            const char* label = theme::roleShortLabel(graph.role(v));
            const Font& font = style.vertexRadius >= 18 ? fonts.bodyBold : fonts.small;
            const float w = measureText(font, label);
            drawText(font, label, center.x - w / 2, center.y - font.baseSize / 2.0f, Fade(WHITE, alpha));
        }

        if (style.showNames && !graph.vertex(v).name.empty()) {
            const char* name = graph.vertex(v).name.c_str();
            const float w = measureText(fonts.small, name);
            drawText(fonts.small, name, center.x - w / 2, center.y + style.vertexRadius + outline + 3,
                     Fade(theme::kTextPrimary, std::max(alpha, 0.45f)));
        }

        // Número da posição do molde (correspondência entre molde e empresa)
        const int badge = valueOr(hl.badge, v, 0);
        if (badge > 0) {
            const std::string text = std::to_string(badge);
            const float r = 9.0f;
            const Vector2 at = {center.x + style.vertexRadius * 0.75f, center.y - style.vertexRadius * 0.75f};
            DrawCircleV(at, r + 1.5f, WHITE);
            DrawCircleV(at, r, theme::kTextPrimary);
            const float w = measureText(fonts.small, text.c_str());
            drawText(fonts.small, text.c_str(), at.x - w / 2, at.y - fonts.small.baseSize / 2.0f - 0.5f, WHITE);
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
