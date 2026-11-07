// Aba Benchmark: gráficos e tabela-resumo a partir de results/bench.csv.

#pragma once

#include <string>
#include <vector>

#include "bench_csv.hpp"
#include "dataset.hpp"
#include "raylib.h"
#include "theme.hpp"

class BenchView {
public:
    // (Re)lê o CSV. `presets` define a ordem e a cor de cada preset (a cor acompanha o preset).
    void load(const std::string& path, const std::vector<Preset>* presets);
    // Desenha a aba inteira abaixo do cabeçalho.
    void draw(const Fonts& fonts, bool inputEnabled);

private:
    struct Point {
        double x = 0;
        double ms = 0;        // mediana das repetições
        bool timeout = false;
        unsigned long long candidates = 0;
        int found = 0;
        int reps = 0;
    };
    struct Series {
        std::string label;
        Color color;
        std::vector<Point> points;
    };

    std::vector<Point> aggregate(const std::string& series, const std::string& preset, const std::string& mode,
                                 const std::string& solver) const;
    void drawChart(const Fonts& fonts, Rectangle area, const char* title, const char* xName, bool logX,
                   const std::vector<Series>& series, const char* modeLabel, const char* note);
    void drawSummary(const Fonts& fonts, Rectangle area, const std::string& solver);

    std::string path_;
    const std::vector<Preset>* presets_ = nullptr;
    std::vector<BenchRow> rows_;
    std::string error_;
    bool loaded_ = false;
    long modTime_ = 0;
    std::vector<std::string> solvers_;  // solvers presentes no CSV
    double limitMs_ = 0;                // tempo limite (deduzido das execuções que estouraram)

    int mode_ = 0;    // 0 = sem solução, 1 = plantada
    int solver_ = 0;

    // Tooltip do ponto sob o mouse (desenhado por último)
    std::vector<std::string> tooltip_;
    Vector2 tooltipAt_{};
};
