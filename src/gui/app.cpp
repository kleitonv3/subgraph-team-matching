#include "app.hpp"

#include <algorithm>
#include <string>

#include "generator.hpp"
#include "graph_view.hpp"
#include "layout.hpp"
#include "raygui.h"
#include "solver.hpp"

namespace {

// --- Disposição da tela (1280×720) -------------------------------------------------------------

constexpr float kHeaderHeight = 56;
const Rectangle kPresetPanel = {12, 68, 270, 560};
const Rectangle kCompanyPanel = {294, 68, 680, 560};
const Rectangle kPatternPanel = {986, 68, 282, 560};
const Rectangle kPlaybackBar = {12, 640, 1256, 68};

constexpr float kCardTop = 102;
constexpr float kCardHeight = 62;
constexpr float kCardGap = 6;
const Rectangle kSolverDropdown = {28, 476, 238, 32};
const Rectangle kCompanyDropdown = {28, 538, 238, 32};
const Rectangle kRunButton = {28, 582, 238, 34};

// Grau médio das empresas geradas na interface: menor que o do benchmark, para o desenho ficar legível.
constexpr double kGuiAverageDegree = 4.0;

// --- Utilidades de desenho ---------------------------------------------------------------------

void drawPanel(Rectangle area) {
    DrawRectangleRounded(area, 0.02f, 8, theme::kPanel);
    DrawRectangleRoundedLinesEx(area, 0.02f, 8, 1.0f, theme::kBorder);
}

void drawSectionLabel(const Fonts& fonts, const char* text, float x, float y) {
    drawText(fonts.small, text, x, y, theme::kTextMuted);
}

// Quebra o texto em linhas de no máximo `width` pixels.
std::vector<std::string> wrapText(const Font& font, const std::string& text, float width) {
    std::vector<std::string> lines;
    std::string line;
    std::size_t start = 0;
    while (start < text.size()) {
        std::size_t end = text.find(' ', start);
        if (end == std::string::npos) end = text.size();
        const std::string word = text.substr(start, end - start);
        const std::string candidate = line.empty() ? word : line + " " + word;
        if (!line.empty() && measureText(font, candidate.c_str()) > width) {
            lines.push_back(line);
            line = word;
        } else {
            line = candidate;
        }
        start = end + 1;
    }
    if (!line.empty()) lines.push_back(line);
    return lines;
}

std::string plural(int count, const char* singular, const char* pluralForm) {
    return std::to_string(count) + " " + (count == 1 ? singular : pluralForm);
}

Rectangle cardRect(int index) {
    return {kPresetPanel.x + 12, kCardTop + index * (kCardHeight + kCardGap), kPresetPanel.width - 24, kCardHeight};
}

}  // namespace

// ---------------------------------------------------------------------------------------------
// Inicialização
// ---------------------------------------------------------------------------------------------

const std::vector<App::CompanyOption>& App::companyOptions() {
    static const std::vector<CompanyOption> options = {
        {"Empresa Exemplo (14 pessoas)", 0},
        {"Gerada · 25 pessoas", 25},
        {"Gerada · 50 pessoas", 50},
        {"Gerada · 100 pessoas", 100},
    };
    return options;
}

void App::init(const std::string& executableDir) {
    std::string assetsDir = executableDir + "../assets";
    if (!DirectoryExists(assetsDir.c_str())) assetsDir = "assets";
    fonts_ = loadFonts(assetsDir.c_str());
    applyGuiStyle(fonts_);

    dataDir_ = findDataDir(executableDir);
    if (!loadPresets(dataDir_, presets_, error_)) return;
    if (!loadGraphFile(dataDir_ + "/company.txt", fileCompany_, error_)) return;

    for (const Preset& preset : presets_) presetLayouts_.push_back(computeLayout(preset.graph));
    selectPreset(0);
}

void App::shutdown() { unloadFonts(fonts_); }

void App::selectPreset(int index) {
    selectedPreset_ = index;
    // As empresas geradas embutem o molde escolhido, então mudam junto com ele
    rebuildCompany();
}

void App::rebuildCompany() {
    const CompanyOption& option = companyOptions()[selectedCompany_];
    if (option.generatedSize == 0) {
        company_ = fileCompany_;
    } else {
        CompanyParams params;
        params.n = option.generatedSize;
        params.averageDegree = kGuiAverageDegree;
        params.mode = CompanyMode::Planted;
        params.seed = 1;
        std::string error;
        if (!generateCompany(params, &presets_[selectedPreset_].graph, company_, error)) {
            error_ = error;
            return;
        }
    }
    companyLayout_ = computeLayout(company_);
    companyRadius_ = vertexRadiusFor(company_.vertexCount());
    hoveredVertex_ = -1;
}

// ---------------------------------------------------------------------------------------------
// Quadro
// ---------------------------------------------------------------------------------------------

void App::frame() {
    BeginDrawing();
    ClearBackground(theme::kBackground);

    if (!error_.empty()) {
        drawError();
        EndDrawing();
        return;
    }

    // Com uma lista de seleção aberta, o resto da tela não recebe cliques
    const bool dropdownOpen = solverDropdownOpen_ || companyDropdownOpen_;
    if (dropdownOpen) GuiLock();

    drawHeader();
    drawPresetPanel(!dropdownOpen);
    drawCompanyView(!dropdownOpen);
    drawPatternPanel();
    drawPlaybackBar();
    drawControls();

    GuiUnlock();
    if (!dropdownOpen) drawTooltip();

    EndDrawing();
}

void App::drawHeader() {
    DrawRectangle(0, 0, GetScreenWidth(), static_cast<int>(kHeaderHeight), theme::kPanel);
    DrawLine(0, static_cast<int>(kHeaderHeight), GetScreenWidth(), static_cast<int>(kHeaderHeight), theme::kBorder);
    drawText(fonts_.title, "Subgraph Team Matching", 20, 16, theme::kTextPrimary);

    // Abas (a de benchmark entra na Fase 6)
    float x = 320;
    const char* tabs[] = {"Visualização", "Benchmark"};
    for (int i = 0; i < 2; ++i) {
        const bool active = i == 0;
        drawText(fonts_.bodyBold, tabs[i], x, 20, active ? theme::kAccent : theme::kTextMuted);
        const float w = measureText(fonts_.bodyBold, tabs[i]);
        if (active) DrawRectangle(static_cast<int>(x), static_cast<int>(kHeaderHeight) - 3, static_cast<int>(w), 3, theme::kAccent);
        x += w + 32;
    }

    const char* hint = "Escolha um molde e uma empresa";
    drawText(fonts_.small, hint, GetScreenWidth() - 20 - measureText(fonts_.small, hint), 21, theme::kTextMuted);
}

void App::drawPresetPanel(bool inputEnabled) {
    drawPanel(kPresetPanel);
    drawSectionLabel(fonts_, "MOLDE DA EQUIPE", kPresetPanel.x + 16, 80);

    const Vector2 mouse = GetMousePosition();
    for (int i = 0; i < static_cast<int>(presets_.size()); ++i) {
        const Rectangle card = cardRect(i);
        if (card.y + card.height > 450) break;  // espaço reservado para os seletores abaixo

        const bool selected = i == selectedPreset_;
        const bool hovered = inputEnabled && CheckCollisionPointRec(mouse, card);
        if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !selected) selectPreset(i);

        DrawRectangleRounded(card, 0.12f, 8,
                             selected ? theme::kCardSelected : (hovered ? theme::kCardHover : theme::kPanel));
        DrawRectangleRoundedLinesEx(card, 0.12f, 8, 1.0f, selected ? theme::kAccent : theme::kBorder);

        // Miniatura do molde
        const Rectangle thumb = {card.x + 6, card.y + 6, 50, 50};
        DrawRectangleRounded(thumb, 0.15f, 6, theme::kBackground);
        GraphViewStyle thumbStyle;
        thumbStyle.vertexRadius = 4.5f;
        thumbStyle.edgeThickness = 1.5f;
        drawGraph(presets_[i].graph, toScreen(presetLayouts_[i], thumb, 8), thumbStyle, fonts_);

        const Graph& g = presets_[i].graph;
        drawText(fonts_.bodyBold, g.name.c_str(), card.x + 66, card.y + 12, theme::kTextPrimary);
        const std::string meta = "k = " + std::to_string(g.vertexCount()) + " · " + plural(g.edgeCount(), "aresta", "arestas");
        drawText(fonts_.small, meta.c_str(), card.x + 66, card.y + 34, theme::kTextSecondary);
    }

    drawSectionLabel(fonts_, "SOLVER", kSolverDropdown.x, kSolverDropdown.y - 20);
    drawSectionLabel(fonts_, "EMPRESA", kCompanyDropdown.x, kCompanyDropdown.y - 20);

    // A execução entra na Fase 5
    GuiDisable();
    GuiButton(kRunButton, GuiIconText(ICON_PLAYER_PLAY, "Executar busca"));
    GuiEnable();
}

void App::drawCompanyView(bool inputEnabled) {
    drawPanel(kCompanyPanel);
    drawText(fonts_.bodyBold, company_.name.c_str(), kCompanyPanel.x + 16, 80, theme::kTextPrimary);
    const std::string meta = plural(company_.vertexCount(), "funcionário", "funcionários") + " · " +
                             plural(company_.edgeCount(), "colaboração", "colaborações");
    drawText(fonts_.small, meta.c_str(), kCompanyPanel.x + 16, 100, theme::kTextSecondary);

    const Rectangle area = {kCompanyPanel.x, 120, kCompanyPanel.width, kCompanyPanel.y + kCompanyPanel.height - 120};
    companyScreen_ = toScreen(companyLayout_, area, 36);
    hoveredVertex_ = inputEnabled ? vertexAt(companyScreen_, companyRadius_, GetMousePosition()) : -1;

    GraphViewStyle style;
    style.vertexRadius = companyRadius_;
    style.edgeThickness = company_.vertexCount() > 40 ? 1.0f : 2.0f;
    style.showNames = company_.vertexCount() <= 30;
    drawGraph(company_, companyScreen_, style, fonts_, hoveredVertex_);
}

void App::drawPatternPanel() {
    const Preset& preset = presets_[selectedPreset_];
    const float x = kPatternPanel.x + 16;
    const float width = kPatternPanel.width - 32;

    drawPanel(kPatternPanel);
    drawSectionLabel(fonts_, "MOLDE", x, 80);
    drawText(fonts_.bodyBold, preset.graph.name.c_str(), x, 100, theme::kTextPrimary);

    const Rectangle view = {x, 124, width, 170};
    DrawRectangleRounded(view, 0.06f, 8, theme::kBackground);
    GraphViewStyle style;
    style.vertexRadius = 18;
    style.edgeThickness = 2.5f;
    style.showRoleLabels = true;
    drawGraph(preset.graph, toScreen(presetLayouts_[selectedPreset_], view, 28), style, fonts_);

    float y = 304;
    for (const std::string& line : wrapText(fonts_.small, preset.graph.description, width)) {
        drawText(fonts_.small, line.c_str(), x, y, theme::kTextSecondary);
        y += 18;
    }

    // Legenda: cor de cada papel e quantos funcionários a empresa tem com esse papel
    drawSectionLabel(fonts_, "PAPÉIS NA EMPRESA", x, 372);
    int counts[kRoleCount] = {};
    for (int v = 0; v < company_.vertexCount(); ++v) ++counts[static_cast<int>(company_.role(v))];
    for (int r = 0; r < kRoleCount; ++r) {
        const Role role = static_cast<Role>(r);
        const float rowY = 394 + r * 22.0f;
        DrawCircle(static_cast<int>(x + 7), static_cast<int>(rowY + 8), 6, theme::roleColor(role));
        drawText(fonts_.body, roleDisplayName(role), x + 22, rowY, theme::kTextPrimary);
        const std::string count = std::to_string(counts[r]);
        drawText(fonts_.body, count.c_str(), x + width - measureText(fonts_.body, count.c_str()), rowY,
                 theme::kTextSecondary);
    }

    drawSectionLabel(fonts_, "EQUIPE FORMADA", x, 560);
    drawText(fonts_.small, "Execute a busca para ver a equipe.", x, 582, theme::kTextMuted);
}

void App::drawPlaybackBar() {
    drawPanel(kPlaybackBar);

    // Controles de reprodução: entram na Fase 5
    GuiDisable();
    const int icons[] = {ICON_PLAYER_PREVIOUS, ICON_ARROW_LEFT_FILL, ICON_PLAYER_PLAY, ICON_ARROW_RIGHT_FILL,
                         ICON_PLAYER_NEXT};
    float x = kPlaybackBar.x + 16;
    const float y = kPlaybackBar.y + 16;
    for (int icon : icons) {
        GuiButton({x, y, 36, 36}, GuiIconText(icon, nullptr));
        x += 42;
    }

    x += 16;
    drawText(fonts_.small, "Velocidade", x, y + 10, theme::kTextMuted);
    x += measureText(fonts_.small, "Velocidade") + 12;
    float speed = 0.3f;
    GuiSliderBar({x, y + 10, 220, 16}, nullptr, nullptr, &speed, 0.0f, 1.0f);
    x += 240;

    drawText(fonts_.body, "Passo — / —", x, y + 9, theme::kTextMuted);

    GuiButton({kPlaybackBar.x + kPlaybackBar.width - 196, y, 180, 36},
              GuiIconText(ICON_PLAYER_NEXT, "Pular para a solução"));
    GuiEnable();
}

void App::drawControls() {
    // Lista de solvers no formato da raygui ("A;B;C")
    std::string solverNames;
    for (const SolverInfo& solver : allSolvers()) {
        if (!solverNames.empty()) solverNames += ";";
        solverNames += solver.displayName;
    }
    std::string companyNames;
    for (const CompanyOption& option : companyOptions()) {
        if (!companyNames.empty()) companyNames += ";";
        companyNames += option.label;
    }

    // A empresa é desenhada antes do solver: a lista do solver abre para baixo, por cima dela
    const int previousCompany = selectedCompany_;
    if (GuiDropdownBox(kCompanyDropdown, companyNames.c_str(), &selectedCompany_, companyDropdownOpen_)) {
        companyDropdownOpen_ = !companyDropdownOpen_;
    }
    if (GuiDropdownBox(kSolverDropdown, solverNames.c_str(), &selectedSolver_, solverDropdownOpen_)) {
        solverDropdownOpen_ = !solverDropdownOpen_;
    }
    if (selectedCompany_ != previousCompany) rebuildCompany();
}

void App::drawTooltip() {
    if (hoveredVertex_ < 0) return;
    const Vertex& vertex = company_.vertex(hoveredVertex_);
    const std::string title = vertex.name.empty() ? "Funcionário #" + std::to_string(hoveredVertex_) : vertex.name;
    const std::string detail = std::string(roleDisplayName(vertex.role)) + " · " +
                               plural(company_.degree(hoveredVertex_), "colaboração", "colaborações");

    const float width = std::max(measureText(fonts_.bodyBold, title.c_str()), measureText(fonts_.small, detail.c_str())) + 24;
    const Vector2 mouse = GetMousePosition();
    Rectangle box = {mouse.x + 14, mouse.y + 14, width, 50};
    if (box.x + box.width > GetScreenWidth() - 8) box.x = mouse.x - 14 - box.width;
    if (box.y + box.height > GetScreenHeight() - 8) box.y = mouse.y - 14 - box.height;

    DrawRectangleRounded(box, 0.2f, 8, theme::kTooltip);
    drawText(fonts_.bodyBold, title.c_str(), box.x + 12, box.y + 8, WHITE);
    drawText(fonts_.small, detail.c_str(), box.x + 12, box.y + 28, Color{220, 224, 232, 255});
}

void App::drawError() {
    const char* title = "Não foi possível carregar os dados";
    const float titleWidth = measureText(fonts_.title, title);
    drawText(fonts_.title, title, (GetScreenWidth() - titleWidth) / 2, 300, theme::kTextPrimary);
    const float errorWidth = measureText(fonts_.body, error_.c_str());
    drawText(fonts_.body, error_.c_str(), (GetScreenWidth() - errorWidth) / 2, 340, theme::kTextSecondary);
    const char* hint = "Rode o programa a partir da pasta do projeto (make run).";
    const float hintWidth = measureText(fonts_.small, hint);
    drawText(fonts_.small, hint, (GetScreenWidth() - hintWidth) / 2, 372, theme::kTextMuted);
}
