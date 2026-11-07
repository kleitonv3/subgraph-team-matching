#include "app.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include "format.hpp"
#include "generator.hpp"
#include "layout.hpp"
#include "raygui.h"

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
// Tempo limite das buscas feitas pela interface (dá para cancelar antes).
constexpr double kGuiTimeLimitSeconds = 60.0;

// --- Utilidades de desenho ---------------------------------------------------------------------

void drawPanel(Rectangle area) {
    DrawRectangleRounded(area, 0.02f, 8, theme::kPanel);
    DrawRectangleRoundedLinesEx(area, 0.02f, 8, 1.0f, theme::kBorder);
}

void drawSectionLabel(const Fonts& fonts, const char* text, float x, float y) {
    drawText(fonts.small, text, x, y, theme::kTextMuted);
}

void drawTextRight(const Font& font, const std::string& text, float right, float y, Color color) {
    drawText(font, text.c_str(), right - measureText(font, text.c_str()), y, color);
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

Rectangle cardRect(int index) {
    return {kPresetPanel.x + 12, kCardTop + index * (kCardHeight + kCardGap), kPresetPanel.width - 24, kCardHeight};
}

std::string personName(const Graph& company, int v) {
    const std::string& name = company.vertex(v).name;
    return name.empty() ? "Funcionário #" + std::to_string(v) : name;
}

}  // namespace

// ---------------------------------------------------------------------------------------------
// Inicialização e seleção
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
    resultsPath_ = dataDir_ + "/../results/bench.csv";
    if (!loadPresets(dataDir_, presets_, error_)) return;
    if (!loadGraphFile(dataDir_ + "/company.txt", fileCompany_, error_)) return;

    for (const Preset& preset : presets_) presetLayouts_.push_back(computeLayout(preset.graph));
    selectPreset(0);
}

void App::shutdown() {
    if (worker_.joinable()) {
        cancel_.store(true);
        worker_.join();
    }
    unloadFonts(fonts_);
}

void App::selectPreset(int index) {
    if (isRunning() || index < 0 || index >= static_cast<int>(presets_.size())) return;
    selectedPreset_ = index;
    // As empresas geradas embutem o molde escolhido, então mudam junto com ele
    rebuildCompany();
}

void App::selectCompany(int index) {
    if (isRunning() || index < 0 || index >= static_cast<int>(companyOptions().size())) return;
    selectedCompany_ = index;
    rebuildCompany();
}

void App::selectTab(int tab) {
    if (tab < 0 || tab > 1) return;
    activeTab_ = tab;
    solverDropdownOpen_ = false;
    companyDropdownOpen_ = false;
    // Relê o CSV ao entrar na aba, para mostrar os resultados de um benchmark recém-rodado
    if (tab == 1) benchView_.load(resultsPath_, &presets_);
}

void App::rebuildCompany() {
    resetRun();
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
// Execução da busca
// ---------------------------------------------------------------------------------------------

void App::resetRun() {
    if (worker_.joinable()) {
        cancel_.store(true);
        worker_.join();
    }
    ctx_.reset();
    result_ = RunResult();
    hasRun_ = false;
    matchStep_ = kNoStep;
    current_ = 0;
    playing_ = false;
    stepAccumulator_ = 0.0;
    showFinal_ = false;
}

void App::startRun() {
    if (isRunning() || !error_.empty()) return;
    resetRun();

    const SolverInfo& solver = allSolvers()[selectedSolver_];
    solverName_ = solver.displayName;

    SearchOptions options;
    options.recordTrace = true;
    options.timeLimitSeconds = kGuiTimeLimitSeconds;
    options.cancel = &cancel_;
    ctx_ = std::make_unique<SearchContext>(options);

    cancel_.store(false);
    running_.store(true, std::memory_order_release);
    runStartTime_ = GetTime();

    // A thread só lê o molde e a empresa (que não mudam enquanto a busca roda: a tela fica travada)
    // e só escreve em result_ e ctx_, que a thread principal lê depois que running_ vira false.
    const Graph* pattern = &presets_[selectedPreset_].graph;
    worker_ = std::thread([this, &solver, pattern]() {
        result_ = runSolver(solver, *pattern, company_, *ctx_);
        running_.store(false, std::memory_order_release);
    });
}

void App::pollRun() {
    if (!worker_.joinable() || isRunning()) return;
    worker_.join();
    hasRun_ = true;

    const Trace& trace = ctx_->trace();
    for (std::size_t i = 0; i < trace.size(); ++i) {
        if (trace.step(i).result == StepResult::Match) {
            matchStep_ = i;
            break;
        }
    }
    current_ = 0;
    playing_ = trace.size() > 1;
}

// ---------------------------------------------------------------------------------------------
// Reprodução
// ---------------------------------------------------------------------------------------------

std::size_t App::lastStep() const {
    const std::size_t size = ctx_ ? ctx_->trace().size() : 0;
    return size == 0 ? 0 : size - 1;
}

float App::stepsPerSecond() const { return std::pow(10.0f, 4.0f * speedSlider_); }

void App::seek(std::size_t step) {
    if (!hasRun_) return;
    current_ = std::min(step, lastStep());
    playing_ = false;
    showFinal_ = false;
    stepAccumulator_ = 0.0;
}

void App::updatePlayback() {
    if (!hasRun_ || !playing_) return;
    stepAccumulator_ += GetFrameTime() * stepsPerSecond();
    const auto advance = static_cast<std::size_t>(stepAccumulator_);
    if (advance == 0) return;
    stepAccumulator_ -= static_cast<double>(advance);

    std::size_t next = std::min(current_ + advance, lastStep());
    // Pausa sozinho quando chega na equipe encontrada
    if (matchStep_ != kNoStep && current_ < matchStep_ && next >= matchStep_) {
        next = matchStep_;
        playing_ = false;
    }
    current_ = next;
    if (current_ >= lastStep()) playing_ = false;
}

bool App::teamRevealed() const {
    if (!hasRun_) return false;
    if (showFinal_ || ctx_->trace().size() == 0) return true;
    if (matchStep_ != kNoStep && current_ >= matchStep_) return true;
    return current_ >= lastStep();
}

bool App::displayedStep(DisplayedStep& step) const {
    if (!hasRun_) return false;
    if (showFinal_ && result_.found) {
        step.map = result_.mapping.data();
        step.k = static_cast<int>(result_.mapping.size());
        step.result = StepResult::Match;
        return true;
    }
    const Trace& trace = ctx_->trace();
    if (trace.size() == 0) return false;
    const TraceStep& info = trace.step(current_);
    step.map = trace.mapping(current_);
    step.k = trace.patternSize();
    step.result = info.result;
    step.failU = info.failU;
    step.failV = info.failV;
    return true;
}

void App::buildHighlights(const DisplayedStep& step, GraphHighlight& companyHl, GraphHighlight& patternHl) const {
    const Graph& pattern = presets_[selectedPreset_].graph;
    const int n = company_.vertexCount();
    const int k = std::min(step.k, pattern.vertexCount());
    const bool match = step.result == StepResult::Match;

    auto assigned = [&](int j) { return j >= 0 && j < k && step.map[j] >= 0 && step.map[j] < n; };
    auto isReportedEdge = [&](int u, int v) {
        return step.failV >= 0 && ((u == step.failU && v == step.failV) || (u == step.failV && v == step.failU));
    };

    companyHl.vertexAlpha.assign(n, 0.25f);
    companyHl.badge.assign(n, 0);
    companyHl.ring.assign(n, Color{0, 0, 0, 0});
    companyHl.edgeAlpha = 0.35f;
    patternHl.badge.assign(pattern.vertexCount(), 0);
    patternHl.ring.assign(pattern.vertexCount(), Color{0, 0, 0, 0});

    // Vértices: cada posição j do molde e o funcionário escolhido para ela recebem o número j + 1
    for (int j = 0; j < pattern.vertexCount(); ++j) {
        patternHl.badge[j] = j + 1;
        if (!assigned(j)) continue;
        const int person = step.map[j];
        const bool roleWrong = company_.role(person) != pattern.role(j) || (step.failU == j && step.failV < 0);
        const Color ring = roleWrong ? theme::kFail : (match ? theme::kOk : Fade(theme::kAccent, 0.6f));

        companyHl.vertexAlpha[person] = 1.0f;
        companyHl.badge[person] = j + 1;
        companyHl.ring[person] = ring;
        if (roleWrong || match) patternHl.ring[j] = roleWrong ? theme::kFail : theme::kOk;
    }

    // Se algum papel não bate, o motivo da falha é o papel: as arestas não são destacadas
    bool rolesOk = !(step.failU >= 0 && step.failV < 0);
    for (int j = 0; j < pattern.vertexCount() && rolesOk; ++j) {
        if (assigned(j) && company_.role(step.map[j]) != pattern.role(j)) rolesOk = false;
    }
    if (!rolesOk) return;

    // Arestas do molde: verde se a colaboração existe na empresa, vermelho (tracejado) se falta
    for (const auto& edge : pattern.edges()) {
        const int u = edge.first;
        const int v = edge.second;
        if (!assigned(u) || !assigned(v)) continue;
        const bool exists = company_.hasEdge(step.map[u], step.map[v]);
        const float bonus = isReportedEdge(u, v) ? 2.0f : 0.0f;
        const Color color = exists ? theme::kOk : theme::kFail;
        companyHl.lines.push_back({step.map[u], step.map[v], color, (exists ? 4.0f : 3.0f) + bonus, !exists});
        patternHl.lines.push_back({u, v, color, 4.0f + bonus, false});
    }
}

// ---------------------------------------------------------------------------------------------
// Quadro
// ---------------------------------------------------------------------------------------------

void App::frame() {
    pollRun();
    updatePlayback();

    BeginDrawing();
    ClearBackground(theme::kBackground);

    if (!error_.empty()) {
        drawError();
        EndDrawing();
        return;
    }

    const bool dropdownOpen = solverDropdownOpen_ || companyDropdownOpen_;
    const bool running = isRunning();

    // Aba Benchmark: só o cabeçalho e os gráficos (a busca, se houver, continua rodando)
    if (activeTab_ == 1) {
        drawHeader();
        benchView_.draw(fonts_, true);
        EndDrawing();
        return;
    }

    // Atalhos de teclado da reprodução
    if (hasRun_ && !dropdownOpen && !running) {
        if (IsKeyPressed(KEY_SPACE)) {
            if (!playing_ && current_ >= lastStep()) seek(0);
            playing_ = !playing_;
        }
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) seek(current_ + 1);
        if ((IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) && current_ > 0) seek(current_ - 1);
        if (IsKeyPressed(KEY_HOME)) seek(0);
        if (IsKeyPressed(KEY_END)) seek(lastStep());
    }

    GraphHighlight companyHl;
    GraphHighlight patternHl;
    DisplayedStep step;
    const bool showStep = displayedStep(step);
    if (showStep) buildHighlights(step, companyHl, patternHl);

    // Com uma lista de seleção aberta, o resto da tela não recebe cliques
    if (dropdownOpen) GuiLock();

    drawHeader();
    drawPresetPanel(!dropdownOpen && !running);
    drawCompanyView(!dropdownOpen, showStep ? &companyHl : nullptr);
    drawPatternPanel(showStep ? &patternHl : nullptr);
    drawPlaybackBar(!dropdownOpen && !running);
    drawControls(!running);

    GuiUnlock();
    if (!dropdownOpen) drawTooltip();

    EndDrawing();
}

void App::drawHeader() {
    DrawRectangle(0, 0, GetScreenWidth(), static_cast<int>(kHeaderHeight), theme::kPanel);
    DrawLine(0, static_cast<int>(kHeaderHeight), GetScreenWidth(), static_cast<int>(kHeaderHeight), theme::kBorder);
    drawText(fonts_.title, "Subgraph Team Matching", 20, 16, theme::kTextPrimary);

    // Abas (clicáveis; com uma lista de seleção aberta, ficam travadas)
    float x = 320;
    const char* tabs[] = {"Visualização", "Benchmark"};
    const bool dropdownOpen = solverDropdownOpen_ || companyDropdownOpen_;
    const Vector2 mouse = GetMousePosition();
    for (int i = 0; i < 2; ++i) {
        const bool active = i == activeTab_;
        const float w = measureText(fonts_.bodyBold, tabs[i]);
        const Rectangle hit = {x - 8, 8, w + 16, kHeaderHeight - 8};
        const bool hovered = !dropdownOpen && CheckCollisionPointRec(mouse, hit);
        if (hovered && !active && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) selectTab(i);
        const Color color = active ? theme::kAccent : (hovered ? theme::kTextSecondary : theme::kTextMuted);
        drawText(fonts_.bodyBold, tabs[i], x, 20, color);
        if (active) DrawRectangle(static_cast<int>(x), static_cast<int>(kHeaderHeight) - 3, static_cast<int>(w), 3, theme::kAccent);
        x += w + 32;
    }

    const char* hint = activeTab_ == 1 ? "Passe o mouse sobre os pontos para ver os detalhes"
                       : hasRun_       ? "Espaço: reproduzir/pausar · ←/→: passo a passo"
                                       : "Escolha um molde e uma empresa e execute a busca";
    drawTextRight(fonts_.small, hint, GetScreenWidth() - 20.0f, 21, theme::kTextMuted);
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
        const std::string meta = "k = " + std::to_string(g.vertexCount()) + " · " + formatCount(g.edgeCount(), "aresta", "arestas");
        drawText(fonts_.small, meta.c_str(), card.x + 66, card.y + 34, theme::kTextSecondary);
    }

    drawSectionLabel(fonts_, "SOLVER", kSolverDropdown.x, kSolverDropdown.y - 20);
    drawSectionLabel(fonts_, "EMPRESA", kCompanyDropdown.x, kCompanyDropdown.y - 20);
}

void App::drawCompanyView(bool inputEnabled, const GraphHighlight* highlight) {
    drawPanel(kCompanyPanel);
    const float left = kCompanyPanel.x + 16;
    const float right = kCompanyPanel.x + kCompanyPanel.width - 16;

    drawText(fonts_.bodyBold, company_.name.c_str(), left, 80, theme::kTextPrimary);
    const std::string meta = formatCount(company_.vertexCount(), "funcionário", "funcionários") + " · " +
                             formatCount(company_.edgeCount(), "colaboração", "colaborações");
    drawText(fonts_.small, meta.c_str(), left, 100, theme::kTextSecondary);

    // Situação da busca, no canto direito
    if (isRunning()) {
        const std::string text = "Buscando… " + formatDuration(GetTime() - runStartTime_);
        drawTextRight(fonts_.bodyBold, text, right, 80, theme::kAccent);
        drawTextRight(fonts_.small, "A tela volta quando a busca terminar", right, 100, theme::kTextMuted);
    } else if (hasRun_) {
        std::string status;
        Color color = theme::kTextPrimary;
        if (!teamRevealed()) {
            status = "Busca concluída";  // o resultado aparece quando a reprodução chegar nele
        } else if (!result_.invalidReason.empty()) {
            status = "Equipe inválida";
            color = theme::kFail;
        } else if (result_.found) {
            status = "Equipe encontrada";
            color = theme::kOk;
        } else if (result_.cancelled) {
            status = "Busca cancelada";
            color = theme::kWarning;
        } else if (result_.timedOut) {
            status = "Tempo limite (" + formatDuration(kGuiTimeLimitSeconds) + ")";
            color = theme::kWarning;
        } else {
            status = "Sem solução";
        }
        drawTextRight(fonts_.bodyBold, status, right, 80, color);
        const std::string stats = solverName_ + " · " + formatCount(result_.steps, "candidato", "candidatos") + " · " +
                                  formatDuration(result_.seconds);
        drawTextRight(fonts_.small, stats, right, 100, theme::kTextSecondary);
    }

    const Rectangle area = {kCompanyPanel.x, 120, kCompanyPanel.width, kCompanyPanel.y + kCompanyPanel.height - 120};
    companyScreen_ = toScreen(companyLayout_, area, 36);
    hoveredVertex_ = inputEnabled ? vertexAt(companyScreen_, companyRadius_, GetMousePosition()) : -1;

    GraphViewStyle style;
    style.vertexRadius = companyRadius_;
    style.edgeThickness = company_.vertexCount() > 40 ? 1.0f : 2.0f;
    style.showNames = company_.vertexCount() <= 30;
    drawGraph(company_, companyScreen_, style, fonts_, hoveredVertex_, highlight);

    // Rodapé do painel: o que aconteceu no passo atual e avisos
    float bottom = kCompanyPanel.y + kCompanyPanel.height - 26;
    if (!hasRun_) return;
    if (result_.steps == 0) {
        drawText(fonts_.small, "O solver não testou nenhum candidato.", left, bottom, theme::kTextMuted);
        return;
    }
    DisplayedStep step;
    if (displayedStep(step)) {
        Color color = theme::kTextSecondary;
        const std::string text = describeStep(step, color);
        drawText(fonts_.bodyBold, text.c_str(), left, bottom - 2, color);
        bottom -= 24;
    }
    if (ctx_->traceTruncated()) {
        const std::string text = "A reprodução mostra os primeiros " + formatThousands(ctx_->trace().size()) +
                                 " passos de " + formatThousands(result_.steps) + ".";
        drawText(fonts_.small, text.c_str(), left, bottom, theme::kWarning);
    }
}

std::string App::describeStep(const DisplayedStep& step, Color& color) const {
    const Graph& pattern = presets_[selectedPreset_].graph;
    const int n = company_.vertexCount();
    auto person = [&](int j) {
        const int v = (j >= 0 && j < step.k) ? step.map[j] : -1;
        return (v >= 0 && v < n) ? personName(company_, v) : "?";
    };

    if (showFinal_) {
        color = theme::kOk;
        return "Equipe encontrada além do trace: todas as colaborações exigidas existem";
    }
    switch (step.result) {
        case StepResult::Match:
            color = theme::kOk;
            return "Equipe válida: papéis certos e todas as colaborações exigidas existem";
        case StepResult::Testing:
            color = theme::kTextSecondary;
            return "Testando candidato…";
        case StepResult::Fail:
            break;
    }

    color = theme::kFail;
    if (step.failU >= 0 && step.failV >= 0) {
        return "Falha: falta a colaboração entre " + person(step.failU) + " (" + std::to_string(step.failU + 1) +
               ") e " + person(step.failV) + " (" + std::to_string(step.failV + 1) + ")";
    }
    auto roleText = [&](int j) {
        return "Falha: " + person(j) + " (" + std::to_string(j + 1) + ") não é " + roleDisplayName(pattern.role(j));
    };
    if (step.failU >= 0 && step.failU < pattern.vertexCount()) return roleText(step.failU);

    // O solver não disse o motivo: mostra o primeiro problema visível no candidato
    for (int j = 0; j < pattern.vertexCount() && j < step.k; ++j) {
        if (step.map[j] >= 0 && step.map[j] < n && company_.role(step.map[j]) != pattern.role(j)) return roleText(j);
    }
    for (const auto& edge : pattern.edges()) {
        const int a = edge.first < step.k ? step.map[edge.first] : -1;
        const int b = edge.second < step.k ? step.map[edge.second] : -1;
        if (a >= 0 && b >= 0 && a < n && b < n && !company_.hasEdge(a, b)) {
            return "Falha: falta a colaboração entre " + person(edge.first) + " e " + person(edge.second);
        }
    }
    return "Falha: candidato descartado";
}

void App::drawPatternPanel(const GraphHighlight* highlight) {
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
    drawGraph(preset.graph, toScreen(presetLayouts_[selectedPreset_], view, 28), style, fonts_, -1, highlight);

    float y = 304;
    for (const std::string& line : wrapText(fonts_.small, preset.graph.description, width)) {
        drawText(fonts_.small, line.c_str(), x, y, theme::kTextSecondary);
        y += 18;
    }

    // Legenda em duas colunas: cor de cada papel e quantos funcionários a empresa tem com ele
    drawSectionLabel(fonts_, "PAPÉIS NA EMPRESA", x, 364);
    int counts[kRoleCount] = {};
    for (int v = 0; v < company_.vertexCount(); ++v) ++counts[static_cast<int>(company_.role(v))];
    const float columnWidth = (width - 16) / 2;
    for (int r = 0; r < kRoleCount; ++r) {
        const Role role = static_cast<Role>(r);
        const float cx = x + (r % 2) * (columnWidth + 16);
        const float cy = 386 + (r / 2) * 22.0f;
        DrawCircleV({cx + 6, cy + 8}, 6, theme::roleColor(role));
        drawText(fonts_.small, roleDisplayName(role), cx + 18, cy + 1, theme::kTextPrimary);
        drawTextRight(fonts_.small, std::to_string(counts[r]), cx + columnWidth, cy + 1, theme::kTextSecondary);
    }

    drawSectionLabel(fonts_, "EQUIPE FORMADA", x, 488);
    drawTeam(x, 510, width);
}

void App::drawTeam(float x, float y, float width) {
    if (isRunning()) {
        drawText(fonts_.small, "Buscando…", x, y, theme::kTextMuted);
        return;
    }
    if (!hasRun_) {
        drawText(fonts_.small, "Execute a busca para ver a equipe.", x, y, theme::kTextMuted);
        return;
    }
    if (!teamRevealed()) {
        for (const std::string& line : wrapText(fonts_.small, "A equipe aparece quando a reprodução chegar nela (ou use \"Pular para a solução\").", width)) {
            drawText(fonts_.small, line.c_str(), x, y, theme::kTextMuted);
            y += 18;
        }
        return;
    }
    if (!result_.invalidReason.empty()) {
        for (const std::string& line : wrapText(fonts_.small, "O solver devolveu uma equipe inválida: " + result_.invalidReason, width)) {
            drawText(fonts_.small, line.c_str(), x, y, theme::kFail);
            y += 18;
        }
        return;
    }
    if (!result_.found) {
        const char* text = result_.cancelled ? "Busca cancelada antes de encontrar uma equipe."
                           : result_.timedOut ? "Tempo limite atingido sem encontrar uma equipe."
                                              : "Nenhuma equipe atende ao molde nesta empresa.";
        for (const std::string& line : wrapText(fonts_.small, text, width)) {
            drawText(fonts_.small, line.c_str(), x, y, theme::kTextSecondary);
            y += 18;
        }
        return;
    }

    const Graph& pattern = presets_[selectedPreset_].graph;
    for (int j = 0; j < pattern.vertexCount() && j < static_cast<int>(result_.mapping.size()); ++j) {
        const Role role = pattern.role(j);
        const std::string label = std::to_string(j + 1) + "  " + roleDisplayName(role);
        DrawCircleV({x + 5, y + 8}, 5, theme::roleColor(role));
        drawText(fonts_.small, label.c_str(), x + 16, y, theme::kTextSecondary);
        drawText(fonts_.bodyBold, personName(company_, result_.mapping[j]).c_str(), x + 120, y - 1, theme::kTextPrimary);
        y += 20;
    }
}

void App::drawPlaybackBar(bool inputEnabled) {
    drawPanel(kPlaybackBar);
    const bool enabled = inputEnabled && hasRun_ && ctx_->trace().size() > 0;
    if (!enabled) GuiDisable();

    const float y = kPlaybackBar.y + 16;
    float x = kPlaybackBar.x + 16;

    // Botões: início, passo anterior, reproduzir/pausar, próximo passo, fim
    if (GuiButton({x, y, 36, 36}, GuiIconText(ICON_PLAYER_PREVIOUS, nullptr))) seek(0);
    x += 42;
    if (GuiButton({x, y, 36, 36}, GuiIconText(ICON_ARROW_LEFT_FILL, nullptr)) && current_ > 0) seek(current_ - 1);
    x += 42;
    if (GuiButton({x, y, 36, 36}, GuiIconText(playing_ ? ICON_PLAYER_PAUSE : ICON_PLAYER_PLAY, nullptr))) {
        if (!playing_ && current_ >= lastStep()) seek(0);
        playing_ = !playing_;
    }
    x += 42;
    if (GuiButton({x, y, 36, 36}, GuiIconText(ICON_ARROW_RIGHT_FILL, nullptr))) seek(current_ + 1);
    x += 42;
    if (GuiButton({x, y, 36, 36}, GuiIconText(ICON_PLAYER_NEXT, nullptr))) seek(lastStep());
    x += 52;

    // Velocidade (escala logarítmica de 1 a 10.000 passos por segundo)
    drawText(fonts_.small, "Velocidade", x, y + 10, theme::kTextMuted);
    x += measureText(fonts_.small, "Velocidade") + 10;
    GuiSliderBar({x, y + 10, 110, 16}, nullptr, nullptr, &speedSlider_, 0.0f, 1.0f);
    x += 118;
    const std::string speed = formatThousands(static_cast<std::uint64_t>(std::lround(stepsPerSecond()))) + "/s";
    drawText(fonts_.small, speed.c_str(), x, y + 10, enabled ? theme::kTextSecondary : theme::kTextMuted);
    x += 70;

    // Linha do tempo: arrastar para ir a qualquer passo
    const float timelineWidth = 330;
    float position = static_cast<float>(current_);
    const float maxPosition = static_cast<float>(std::max<std::size_t>(lastStep(), 1));
    GuiSliderBar({x, y + 10, timelineWidth, 16}, nullptr, nullptr, &position, 0.0f, maxPosition);
    if (enabled && std::fabs(position - static_cast<float>(current_)) >= 1.0f) {
        seek(static_cast<std::size_t>(position));
    }
    x += timelineWidth + 14;

    const std::string stepText = hasRun_ && ctx_->trace().size() > 0
                                     ? "Passo " + formatThousands(current_ + 1) + " / " + formatThousands(ctx_->trace().size())
                                     : "Passo — / —";
    drawText(fonts_.small, stepText.c_str(), x, y + 10, enabled ? theme::kTextPrimary : theme::kTextMuted);

    // Pular para a solução: vai até a equipe no trace; se ela ficou além do limite do trace, mostra
    // direto a equipe final
    const bool canJump = enabled && result_.found;
    if (!canJump) GuiDisable();
    if (GuiButton({kPlaybackBar.x + kPlaybackBar.width - 196, y, 180, 36},
                  GuiIconText(ICON_PLAYER_NEXT, "Pular para a solução"))) {
        if (matchStep_ != kNoStep) {
            seek(matchStep_);
        } else {
            seek(lastStep());
            showFinal_ = true;
        }
    }
    GuiEnable();
}

void App::drawControls(bool inputEnabled) {
    // Executar / Cancelar
    if (isRunning()) {
        if (GuiButton(kRunButton, GuiIconText(ICON_PLAYER_STOP, "Cancelar"))) cancel_.store(true);
    } else if (GuiButton(kRunButton, GuiIconText(ICON_PLAYER_PLAY, "Executar busca"))) {
        startRun();
    }

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

    if (!inputEnabled) GuiDisable();
    // A empresa é desenhada antes do solver: a lista do solver abre para baixo, por cima dela
    int company = selectedCompany_;
    if (GuiDropdownBox(kCompanyDropdown, companyNames.c_str(), &company, companyDropdownOpen_)) {
        companyDropdownOpen_ = !companyDropdownOpen_;
    }
    int solver = selectedSolver_;
    if (GuiDropdownBox(kSolverDropdown, solverNames.c_str(), &solver, solverDropdownOpen_)) {
        solverDropdownOpen_ = !solverDropdownOpen_;
    }
    GuiEnable();

    if (company != selectedCompany_) selectCompany(company);
    if (solver != selectedSolver_) {
        selectedSolver_ = solver;
        resetRun();
    }
}

void App::drawTooltip() {
    if (hoveredVertex_ < 0) return;
    const Vertex& vertex = company_.vertex(hoveredVertex_);
    const std::string title = personName(company_, hoveredVertex_);
    const std::string detail = std::string(roleDisplayName(vertex.role)) + " · " +
                               formatCount(company_.degree(hoveredVertex_), "colaboração", "colaborações");

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
