// Estado e desenho da tela principal da interface.

#pragma once

#include <string>
#include <vector>

#include "dataset.hpp"
#include "graph.hpp"
#include "raylib.h"
#include "theme.hpp"

class App {
public:
    // Carrega fontes e dados. Se os dados não carregarem, a tela mostra a mensagem de erro.
    void init(const std::string& executableDir);
    // Processa a entrada e desenha um quadro.
    void frame();
    void shutdown();

private:
    // Empresas disponíveis no seletor: a de demonstração (arquivo) ou geradas com o molde embutido.
    struct CompanyOption {
        const char* label;
        int generatedSize;  // 0 = data/company.txt
    };
    static const std::vector<CompanyOption>& companyOptions();

    void selectPreset(int index);
    void rebuildCompany();

    void drawHeader();
    void drawPresetPanel(bool inputEnabled);
    void drawCompanyView(bool inputEnabled);
    void drawPatternPanel();
    void drawPlaybackBar();
    void drawControls();  // seletores de solver e empresa (por último, para a lista abrir por cima)
    void drawTooltip();
    void drawError();

    Fonts fonts_{};
    std::string dataDir_;
    std::string error_;

    std::vector<Preset> presets_;
    std::vector<std::vector<Vector2>> presetLayouts_;
    Graph fileCompany_;
    Graph company_;
    std::vector<Vector2> companyLayout_;
    std::vector<Vector2> companyScreen_;
    float companyRadius_ = 16.0f;

    int selectedPreset_ = 0;
    int selectedCompany_ = 0;
    int selectedSolver_ = 0;
    bool solverDropdownOpen_ = false;
    bool companyDropdownOpen_ = false;
    int hoveredVertex_ = -1;
};
