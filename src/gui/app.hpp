// Estado e desenho da tela principal da interface.

#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "bench_view.hpp"
#include "dataset.hpp"
#include "graph.hpp"
#include "graph_view.hpp"
#include "raylib.h"
#include "solver.hpp"
#include "theme.hpp"

class App {
public:
    App() = default;
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    // Carrega fontes e dados. Se os dados não carregarem, a tela mostra a mensagem de erro.
    void init(const std::string& executableDir);
    // Processa a entrada e desenha um quadro.
    void frame();
    // Cancela uma busca em andamento e libera os recursos.
    void shutdown();

    // --- Ações (usadas pelos controles da tela) ---------------------------------------------

    void selectPreset(int index);
    void selectCompany(int index);
    // 0 = Visualização, 1 = Benchmark (relê results/bench.csv).
    void selectTab(int tab);
    // Roda o solver selecionado em outra thread, gravando o trace.
    void startRun();
    bool isRunning() const { return running_.load(std::memory_order_acquire); }
    // Vai para o passo `step` do trace (e pausa a reprodução).
    void seek(std::size_t step);

private:
    // Empresas disponíveis no seletor: a de demonstração (arquivo) ou geradas com o molde embutido.
    struct CompanyOption {
        const char* label;
        int generatedSize;  // 0 = data/company.txt
    };
    static const std::vector<CompanyOption>& companyOptions();

    // O que está sendo mostrado no momento: um passo do trace ou a equipe final.
    struct DisplayedStep {
        const int* map = nullptr;
        int k = 0;
        StepResult result = StepResult::Testing;
        int failU = -1;
        int failV = -1;
    };

    void rebuildCompany();
    void resetRun();
    void pollRun();
    void updatePlayback();
    bool displayedStep(DisplayedStep& step) const;
    // Frase que explica o passo (e a cor para mostrá-la).
    std::string describeStep(const DisplayedStep& step, Color& color) const;
    void buildHighlights(const DisplayedStep& step, GraphHighlight& company, GraphHighlight& pattern) const;
    std::size_t lastStep() const;
    float stepsPerSecond() const;
    bool teamRevealed() const;

    void drawHeader();
    void drawPresetPanel(bool inputEnabled);
    void drawCompanyView(bool inputEnabled, const GraphHighlight* highlight);
    void drawPatternPanel(const GraphHighlight* highlight);
    void drawTeam(float x, float y, float width);
    void drawPlaybackBar(bool inputEnabled);
    void drawControls(bool inputEnabled);  // seletores (por último, para a lista abrir por cima)
    void drawTooltip();
    void drawError();

    Fonts fonts_{};
    std::string dataDir_;
    std::string error_;

    // Abas: 0 = Visualização, 1 = Benchmark
    int activeTab_ = 0;
    BenchView benchView_;
    std::string resultsPath_;

    // Dados
    std::vector<Preset> presets_;
    std::vector<std::vector<Vector2>> presetLayouts_;
    Graph fileCompany_;
    Graph company_;
    std::vector<Vector2> companyLayout_;
    std::vector<Vector2> companyScreen_;
    float companyRadius_ = 16.0f;

    // Seleção
    int selectedPreset_ = 0;
    int selectedCompany_ = 0;
    int selectedSolver_ = 0;
    bool solverDropdownOpen_ = false;
    bool companyDropdownOpen_ = false;
    int hoveredVertex_ = -1;

    // Execução (a busca roda em outra thread; os resultados só são lidos depois que ela termina)
    std::thread worker_;
    std::atomic<bool> running_{false};
    std::atomic<bool> cancel_{false};
    std::unique_ptr<SearchContext> ctx_;
    RunResult result_;
    std::string solverName_;
    bool hasRun_ = false;
    double runStartTime_ = 0.0;
    static constexpr std::size_t kNoStep = static_cast<std::size_t>(-1);
    std::size_t matchStep_ = kNoStep;  // primeiro Match dentro do trace

    // Reprodução
    std::size_t current_ = 0;
    bool playing_ = false;
    float speedSlider_ = 0.25f;     // 0..1, escala logarítmica de 1 a 10.000 passos/s
    double stepAccumulator_ = 0.0;
    bool showFinal_ = false;        // mostrando a equipe encontrada além do fim do trace
};
