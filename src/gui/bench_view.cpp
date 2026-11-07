#include "bench_view.hpp"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <map>
#include <set>

#include "format.hpp"
#include "raygui.h"
#include "solver.hpp"

namespace {

// Paleta categórica (uma cor por preset, em ordem fixa). Validada com o validador da skill de
// visualização: separação para daltonismo e visão normal OK; contraste baixo em 3 cores, por isso
// os gráficos têm legenda, tooltip e a tabela-resumo.
const Color kSeriesColors[] = {
    {42, 120, 214, 255},   // azul
    {235, 104, 52, 255},   // laranja
    {27, 175, 122, 255},   // verde-água
    {237, 161, 0, 255},    // amarelo
    {232, 123, 164, 255},  // magenta
    {0, 131, 0, 255},      // verde
};
constexpr int kSeriesColorCount = sizeof(kSeriesColors) / sizeof(kSeriesColors[0]);
// Série k crescente: neutra, para não repetir a cor de nenhum preset
const Color kGrowKColor = {73, 80, 87, 255};

const Rectangle kFilterRow = {12, 68, 1256, 36};
const Rectangle kGrowNPanel = {12, 112, 820, 362};
const Rectangle kGrowKPanel = {844, 112, 424, 362};
const Rectangle kSummaryPanel = {12, 486, 1256, 222};

void drawPanel(Rectangle area) {
    DrawRectangleRounded(area, 0.02f, 8, theme::kPanel);
    DrawRectangleRoundedLinesEx(area, 0.02f, 8, 1.0f, theme::kBorder);
}

// Rótulo de tempo para o eixo: 0,1 µs ... 100 µs, 1 ms ... 100 ms, 1 s ...
std::string timeLabel(double ms) {
    char buffer[32];
    auto fmt = [&](double value, const char* unit) {
        if (value >= 1) std::snprintf(buffer, sizeof buffer, "%.0f %s", value, unit);
        else std::snprintf(buffer, sizeof buffer, "%.1f %s", value, unit);
        std::string text = buffer;
        std::replace(text.begin(), text.end(), '.', ',');
        return text;
    };
    if (ms < 1.0) return fmt(ms * 1000.0, "µs");
    if (ms < 1000.0) return fmt(ms, "ms");
    return fmt(ms / 1000.0, "s");
}

void drawDashed(Vector2 a, Vector2 b, float thickness, Color color) {
    const float dx = b.x - a.x, dy = b.y - a.y;
    const float length = std::sqrt(dx * dx + dy * dy);
    for (float t = 0; t < length; t += 10) {
        const float e = std::min(t + 6, length);
        DrawLineEx({a.x + dx * t / length, a.y + dy * t / length}, {a.x + dx * e / length, a.y + dy * e / length},
                   thickness, color);
    }
}

}  // namespace

// ---------------------------------------------------------------------------------------------
// Dados
// ---------------------------------------------------------------------------------------------

void BenchView::load(const std::string& path, const std::vector<Preset>* presets) {
    path_ = path;
    presets_ = presets;
    rows_.clear();
    solvers_.clear();
    error_.clear();
    limitMs_ = 0;
    loaded_ = readBenchCsv(path, rows_, error_);
    modTime_ = loaded_ ? GetFileModTime(path.c_str()) : 0;

    // Solvers na ordem do registro
    std::set<std::string> present;
    for (const BenchRow& row : rows_) present.insert(row.solver);
    for (const SolverInfo& solver : allSolvers()) {
        if (present.count(solver.id)) solvers_.push_back(solver.id);
    }
    for (const std::string& id : present) {
        if (std::find(solvers_.begin(), solvers_.end(), id) == solvers_.end()) solvers_.push_back(id);
    }
    solver_ = 0;

    for (const BenchRow& row : rows_) {
        if (row.timeout) limitMs_ = std::max(limitMs_, row.timeMs);
    }
}

std::vector<BenchView::Point> BenchView::aggregate(const std::string& series, const std::string& preset,
                                                   const std::string& mode, const std::string& solver) const {
    std::map<int, std::vector<const BenchRow*>> bySize;
    for (const BenchRow& row : rows_) {
        if (row.series != series || row.mode != mode || row.solver != solver) continue;
        if (!preset.empty() && row.preset != preset) continue;
        bySize[series == "grow_n" ? row.n : row.k].push_back(&row);
    }

    std::vector<Point> points;
    for (const auto& entry : bySize) {
        Point p;
        p.x = entry.first;
        std::vector<double> times;
        std::vector<unsigned long long> candidates;
        for (const BenchRow* row : entry.second) {
            times.push_back(row->timeMs);
            candidates.push_back(row->candidates);
            p.timeout = p.timeout || row->timeout;
            p.found += row->found ? 1 : 0;
        }
        std::sort(times.begin(), times.end());
        std::sort(candidates.begin(), candidates.end());
        p.ms = times[times.size() / 2];
        p.candidates = candidates[candidates.size() / 2];
        p.reps = static_cast<int>(entry.second.size());
        points.push_back(p);
    }
    return points;
}

// ---------------------------------------------------------------------------------------------
// Desenho
// ---------------------------------------------------------------------------------------------

void BenchView::draw(const Fonts& fonts, bool inputEnabled) {
    tooltip_.clear();
    if (!inputEnabled) GuiLock();

    // Linha de filtros
    drawText(fonts.small, "MODO", kFilterRow.x + 4, kFilterRow.y + 10, theme::kTextMuted);
    GuiToggleGroup({kFilterRow.x + 52, kFilterRow.y + 2, 130, 32}, "Sem solução;Plantada", &mode_);
    float x = kFilterRow.x + 52 + 2 * 130 + 24;
    if (solvers_.size() > 1) {
        drawText(fonts.small, "SOLVER", x, kFilterRow.y + 10, theme::kTextMuted);
        std::string names;
        for (const std::string& id : solvers_) {
            const SolverInfo* info = findSolver(id);
            if (!names.empty()) names += ";";
            names += info ? info->displayName : id;
        }
        GuiToggleGroup({x + 60, kFilterRow.y + 2, 130, 32}, names.c_str(), &solver_);
    }

    const float right = kFilterRow.x + kFilterRow.width;
    if (GuiButton({right - 130, kFilterRow.y + 2, 130, 32}, GuiIconText(ICON_REDO, "Recarregar"))) load(path_, presets_);
    std::string info = "results/bench.csv";
    if (loaded_ && modTime_ > 0) {
        const std::time_t t = static_cast<std::time_t>(modTime_);
        char when[32];
        std::strftime(when, sizeof when, "%d/%m %H:%M", std::localtime(&t));
        info = "results/bench.csv · atualizado em " + std::string(when);
    }
    drawText(fonts.small, info.c_str(), right - 146 - measureText(fonts.small, info.c_str()), kFilterRow.y + 10,
             theme::kTextMuted);

    if (!inputEnabled) GuiUnlock();

    // Sem dados ainda
    if (!loaded_ || rows_.empty() || solvers_.empty()) {
        const Rectangle area = {12, 112, 1256, 596};
        drawPanel(area);
        const char* title = "Nenhum resultado de benchmark ainda";
        drawText(fonts.title, title, area.x + (area.width - measureText(fonts.title, title)) / 2, 330, theme::kTextPrimary);
        const char* hint = "Rode  make bench  no terminal (leva alguns minutos) e clique em Recarregar.";
        drawText(fonts.body, hint, area.x + (area.width - measureText(fonts.body, hint)) / 2, 368, theme::kTextSecondary);
        if (loaded_ && rows_.empty()) {
            const char* empty = "O arquivo existe, mas não tem resultados.";
            drawText(fonts.small, empty, area.x + (area.width - measureText(fonts.small, empty)) / 2, 398, theme::kTextMuted);
        } else if (!loaded_ && error_.find("não encontrado") == std::string::npos) {
            drawText(fonts.small, error_.c_str(), area.x + (area.width - measureText(fonts.small, error_.c_str())) / 2, 398,
                     theme::kFail);
        }
        return;
    }

    const std::string& solver = solvers_[std::min<std::size_t>(solver_, solvers_.size() - 1)];
    const char* mode = mode_ == 0 ? "nosolution" : "planted";
    const char* modeLabel = mode_ == 0 ? "sem solução" : "plantada";

    // Série n crescente: uma linha por preset (a cor acompanha o preset)
    std::vector<Series> growN;
    if (presets_) {
        for (std::size_t i = 0; i < presets_->size(); ++i) {
            const Preset& preset = (*presets_)[i];
            Series s;
            s.label = preset.graph.name + " (k=" + std::to_string(preset.graph.vertexCount()) + ")";
            s.color = kSeriesColors[i % kSeriesColorCount];
            s.points = aggregate("grow_n", preset.id, mode, solver);
            if (!s.points.empty()) growN.push_back(s);
        }
    }
    // No pior caso, a quantidade de candidatos só depende de n e k: linhas de moldes com o mesmo k se sobrepõem
    const char* overlapNote =
        mode_ == 0 ? "No pior caso a força bruta testa todos os n!/(n-k)! candidatos: moldes com o mesmo k se sobrepõem."
                   : nullptr;
    drawChart(fonts, kGrowNPanel, "Tempo × tamanho da empresa (k fixo)", "n (funcionários)", true, growN, modeLabel,
              overlapNote);

    // Série k crescente: uma linha só
    std::vector<Series> growK;
    Series s;
    int fixedN = 0;
    for (const BenchRow& row : rows_) {
        if (row.series == "grow_k") fixedN = row.n;
    }
    s.label = "Moldes aleatórios";
    s.color = kGrowKColor;
    s.points = aggregate("grow_k", "", mode, solver);
    if (!s.points.empty()) growK.push_back(s);
    const std::string kTitle = "Tempo × tamanho do molde (n = " + std::to_string(fixedN) + ")";
    drawChart(fonts, kGrowKPanel, kTitle.c_str(), "k (vértices do molde)", false, growK, modeLabel, nullptr);

    drawSummary(fonts, kSummaryPanel, solver);

    // Tooltip por cima de tudo
    if (!tooltip_.empty() && inputEnabled) {
        float width = 0;
        for (std::size_t i = 0; i < tooltip_.size(); ++i) {
            width = std::max(width, measureText(i == 0 ? fonts.bodyBold : fonts.small, tooltip_[i].c_str()));
        }
        Rectangle box = {tooltipAt_.x + 14, tooltipAt_.y - 10, width + 24, 14.0f + 20.0f * tooltip_.size()};
        if (box.x + box.width > GetScreenWidth() - 8) box.x = tooltipAt_.x - 14 - box.width;
        if (box.y + box.height > GetScreenHeight() - 8) box.y = GetScreenHeight() - 8 - box.height;
        DrawRectangleRounded(box, 0.15f, 8, theme::kTooltip);
        for (std::size_t i = 0; i < tooltip_.size(); ++i) {
            drawText(i == 0 ? fonts.bodyBold : fonts.small, tooltip_[i].c_str(), box.x + 12, box.y + 8 + 20.0f * i,
                     i == 0 ? WHITE : Color{220, 224, 232, 255});
        }
    }
}

void BenchView::drawChart(const Fonts& fonts, Rectangle area, const char* title, const char* xName, bool logX,
                          const std::vector<Series>& series, const char* modeLabel, const char* note) {
    drawPanel(area);
    drawText(fonts.bodyBold, title, area.x + 16, area.y + 12, theme::kTextPrimary);

    // Legenda (só com 2 séries ou mais; com uma, o título já diz o que é)
    float legendY = area.y + 36;
    if (series.size() >= 2) {
        float lx = area.x + 16;
        for (const Series& s : series) {
            const float w = measureText(fonts.small, s.label.c_str());
            if (lx + 22 + w > area.x + area.width - 16) {
                lx = area.x + 16;
                legendY += 18;
            }
            DrawLineEx({lx, legendY + 8}, {lx + 16, legendY + 8}, 2.0f, s.color);
            DrawCircleV({lx + 8, legendY + 8}, 3.5f, s.color);
            drawText(fonts.small, s.label.c_str(), lx + 22, legendY, theme::kTextSecondary);
            lx += 22 + w + 18;
        }
        legendY += 18;
    }
    if (note) {
        drawText(fonts.small, note, area.x + 16, legendY + 2, theme::kTextMuted);
        legendY += 18;
    }

    const Rectangle plot = {area.x + 70, legendY + 14, area.width - 70 - 24, area.y + area.height - 44 - (legendY + 14)};
    if (series.empty()) {
        const char* empty = "Sem dados para este modo.";
        drawText(fonts.small, empty, plot.x + (plot.width - measureText(fonts.small, empty)) / 2, plot.y + plot.height / 2,
                 theme::kTextMuted);
        return;
    }

    // Escalas: y em log10 (tempo), x em log2 (n) ou linear (k)
    double minMs = 1e18, maxMs = 0, minX = 1e18, maxX = 0;
    for (const Series& s : series) {
        for (const Point& p : s.points) {
            minMs = std::min(minMs, std::max(p.ms, 1e-4));
            maxMs = std::max(maxMs, p.ms);
            minX = std::min(minX, p.x);
            maxX = std::max(maxX, p.x);
        }
    }
    maxMs = std::max(maxMs, limitMs_);
    const double yLo = std::pow(10.0, std::floor(std::log10(minMs)));
    double yHi = std::pow(10.0, std::ceil(std::log10(maxMs * 1.0001)));
    if (yHi <= yLo * 10) yHi = yLo * 100;
    if (maxX <= minX) maxX = minX + 1;

    auto px = [&](double x) {
        const double t = logX ? (std::log2(x) - std::log2(minX)) / (std::log2(maxX) - std::log2(minX))
                              : (x - minX) / (maxX - minX);
        return static_cast<float>(plot.x + t * plot.width);
    };
    auto py = [&](double ms) {
        const double t = (std::log10(std::max(ms, yLo)) - std::log10(yLo)) / (std::log10(yHi) - std::log10(yLo));
        return static_cast<float>(plot.y + plot.height - t * plot.height);
    };

    // Grade e eixo y (potências de 10), recessivos
    for (double v = yLo; v <= yHi * 1.0001; v *= 10) {
        const float y = py(v);
        DrawLineEx({plot.x, y}, {plot.x + plot.width, y}, 1.0f, theme::kBorder);
        const std::string label = timeLabel(v);
        drawText(fonts.small, label.c_str(), plot.x - 10 - measureText(fonts.small, label.c_str()), y - 8, theme::kTextMuted);
    }
    DrawLineEx({plot.x, plot.y + plot.height}, {plot.x + plot.width, plot.y + plot.height}, 1.0f, theme::kTextMuted);

    // Eixo x: um rótulo por tamanho medido (pulando rótulos que ficariam colados)
    std::set<double> xs;
    for (const Series& s : series) {
        for (const Point& p : s.points) xs.insert(p.x);
    }
    float lastLabelEnd = -1e9f;
    for (double x : xs) {
        const std::string label = std::to_string(static_cast<long long>(x));
        const float w = measureText(fonts.small, label.c_str());
        const float cx = px(x);
        DrawLineEx({cx, plot.y + plot.height}, {cx, plot.y + plot.height + 4}, 1.0f, theme::kTextMuted);
        if (cx - w / 2 > lastLabelEnd + 6) {
            drawText(fonts.small, label.c_str(), cx - w / 2, plot.y + plot.height + 6, theme::kTextMuted);
            lastLabelEnd = cx + w / 2;
        }
    }
    drawText(fonts.small, xName, plot.x + plot.width - measureText(fonts.small, xName), plot.y + plot.height + 24,
             theme::kTextSecondary);
    const std::string modeText = std::string("modo: ") + modeLabel;
    drawText(fonts.small, modeText.c_str(), plot.x, plot.y + plot.height + 24, theme::kTextMuted);

    // Linha do tempo limite
    if (limitMs_ > 0) {
        const float y = py(limitMs_);
        drawDashed({plot.x, y}, {plot.x + plot.width, y}, 1.5f, theme::kTextSecondary);
        const std::string label = "tempo limite (" + timeLabel(limitMs_) + ")";
        drawText(fonts.small, label.c_str(), plot.x + plot.width - measureText(fonts.small, label.c_str()), y - 18,
                 theme::kTextSecondary);
    }

    // Séries: linhas de 2 px, marcadores de 8 px com anel da cor do fundo.
    // Execuções que estouraram o tempo limite: marcador vazado, ligado por linha tracejada.
    const Vector2 mouse = GetMousePosition();
    float bestDistance = 14.0f;
    const Series* hoveredSeries = nullptr;
    const Point* hoveredPoint = nullptr;
    for (const Series& s : series) {
        for (std::size_t i = 1; i < s.points.size(); ++i) {
            const Point& a = s.points[i - 1];
            const Point& b = s.points[i];
            const Vector2 pa = {px(a.x), py(a.ms)};
            const Vector2 pb = {px(b.x), py(b.ms)};
            if (b.timeout) drawDashed(pa, pb, 2.0f, s.color);
            else DrawLineEx(pa, pb, 2.0f, s.color);
        }
        for (const Point& p : s.points) {
            const Vector2 c = {px(p.x), py(p.ms)};
            DrawCircleV(c, 6.0f, theme::kPanel);
            if (p.timeout) {
                DrawRing(c, 2.5f, 4.5f, 0, 360, 24, s.color);
            } else {
                DrawCircleV(c, 4.0f, s.color);
            }
            const float d = std::hypot(mouse.x - c.x, mouse.y - c.y);
            if (d < bestDistance) {
                bestDistance = d;
                hoveredSeries = &s;
                hoveredPoint = &p;
            }
        }
    }

    // Hover: linha guia vertical + tooltip
    if (hoveredPoint && CheckCollisionPointRec(mouse, area)) {
        const float cx = px(hoveredPoint->x);
        DrawLineEx({cx, plot.y}, {cx, plot.y + plot.height}, 1.0f, Fade(theme::kTextMuted, 0.6f));
        DrawCircleV({cx, py(hoveredPoint->ms)}, 6.5f, Fade(hoveredSeries->color, 0.35f));

        tooltip_.clear();
        tooltip_.push_back(hoveredSeries->label);
        tooltip_.push_back(std::string(logX ? "n = " : "k = ") + std::to_string(static_cast<long long>(hoveredPoint->x)) +
                           " · " + modeLabel);
        if (hoveredPoint->timeout) {
            tooltip_.push_back("Estourou o tempo limite (" + timeLabel(limitMs_) + ")");
        } else {
            tooltip_.push_back(formatDuration(hoveredPoint->ms / 1000.0) + " · " +
                               formatCount(hoveredPoint->candidates, "candidato", "candidatos"));
            if (hoveredPoint->reps > 1) {
                tooltip_.push_back("Mediana de " + std::to_string(hoveredPoint->reps) + " execuções (" +
                                   std::to_string(hoveredPoint->found) + " acharam a equipe)");
            }
        }
        tooltipAt_ = mouse;
    }
}

void BenchView::drawSummary(const Fonts& fonts, Rectangle area, const std::string& solver) {
    drawPanel(area);
    drawText(fonts.bodyBold, "Até que tamanho a busca é viável?", area.x + 16, area.y + 12, theme::kTextPrimary);
    const std::string subtitle = "Maior tamanho resolvido dentro do tempo limite" +
                                 (limitMs_ > 0 ? " (" + timeLabel(limitMs_) + ")" : std::string()) +
                                 ". \"≥\" indica que o limite não foi atingido até o maior tamanho testado.";
    drawText(fonts.small, subtitle.c_str(), area.x + 16, area.y + 32, theme::kTextSecondary);

    const float columns[] = {area.x + 16, area.x + 420, area.x + 520, area.x + 760, area.x + 1000};
    const char* headers[] = {"Molde", "k", "Sem solução (pior caso)", "Plantada (até achar)", "Candidatos no maior n viável"};
    const float headerY = area.y + 60;
    for (int c = 0; c < 5; ++c) drawText(fonts.small, headers[c], columns[c], headerY, theme::kTextMuted);
    DrawLineEx({area.x + 16, headerY + 20}, {area.x + area.width - 16, headerY + 20}, 1.0f, theme::kBorder);

    // Maior tamanho sem estouro; "≥" se nunca estourou
    auto largest = [&](const std::vector<Point>& points, unsigned long long* candidates) {
        double best = 0;
        bool timedOut = false;
        for (const Point& p : points) {
            if (p.timeout) timedOut = true;
            else if (p.x > best) {
                best = p.x;
                if (candidates) *candidates = p.candidates;
            }
        }
        if (points.empty()) return std::string("—");
        if (best == 0) return std::string("nenhum");
        return std::string(timedOut ? "" : "≥ ") + std::to_string(static_cast<long long>(best));
    };

    float y = headerY + 28;
    auto row = [&](Color swatch, const std::string& name, const std::string& k, const std::vector<Point>& noSolution,
                   const std::vector<Point>& planted, const char* unit) {
        DrawCircleV({columns[0] + 5, y + 8}, 5, swatch);
        drawText(fonts.body, name.c_str(), columns[0] + 16, y, theme::kTextPrimary);
        drawText(fonts.body, k.c_str(), columns[1], y, theme::kTextSecondary);
        unsigned long long candidates = 0;
        const std::string worst = largest(noSolution, &candidates);
        const std::string plant = largest(planted, nullptr);
        drawText(fonts.bodyBold, (worst == "—" || worst == "nenhum" ? worst : std::string(unit) + worst).c_str(), columns[2], y,
                 theme::kTextPrimary);
        drawText(fonts.body, (plant == "—" || plant == "nenhum" ? plant : std::string(unit) + plant).c_str(), columns[3], y,
                 theme::kTextPrimary);
        drawText(fonts.body, candidates > 0 ? formatThousands(candidates).c_str() : "—", columns[4], y,
                 theme::kTextSecondary);
        y += 22;
    };

    if (presets_) {
        for (std::size_t i = 0; i < presets_->size(); ++i) {
            const Preset& preset = (*presets_)[i];
            const auto noSolution = aggregate("grow_n", preset.id, "nosolution", solver);
            const auto planted = aggregate("grow_n", preset.id, "planted", solver);
            if (noSolution.empty() && planted.empty()) continue;
            row(kSeriesColors[i % kSeriesColorCount], preset.graph.name, std::to_string(preset.graph.vertexCount()),
                noSolution, planted, "n ");
        }
    }
    const auto noSolutionK = aggregate("grow_k", "", "nosolution", solver);
    const auto plantedK = aggregate("grow_k", "", "planted", solver);
    if (!noSolutionK.empty() || !plantedK.empty()) {
        int fixedN = 0;
        for (const BenchRow& r : rows_) {
            if (r.series == "grow_k") fixedN = r.n;
        }
        row(kGrowKColor, "Moldes aleatórios (n = " + std::to_string(fixedN) + ")", "varia", noSolutionK, plantedK, "k ");
    }
}
