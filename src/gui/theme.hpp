// Cores, fontes e estilo visual da interface.

#pragma once

#include "graph.hpp"
#include "raylib.h"

namespace theme {

// Fundo e superfícies
const Color kBackground = {245, 246, 250, 255};
const Color kPanel = {255, 255, 255, 255};
const Color kBorder = {226, 229, 236, 255};
const Color kCardHover = {243, 246, 252, 255};
const Color kCardSelected = {234, 240, 253, 255};

// Texto
const Color kTextPrimary = {30, 34, 45, 255};
const Color kTextSecondary = {90, 98, 115, 255};
const Color kTextMuted = {150, 157, 170, 255};

// Destaque e grafos
const Color kAccent = {59, 111, 224, 255};
const Color kEdge = {190, 196, 206, 255};
const Color kVertexOutline = {255, 255, 255, 255};
const Color kTooltip = {30, 34, 45, 235};

// Resultado de cada passo da busca
const Color kOk = {47, 158, 68, 255};     // colaboração exigida que existe / equipe válida
const Color kFail = {224, 49, 49, 255};   // colaboração que falta / papel errado
const Color kWarning = {230, 119, 0, 255};

// Cor de cada papel (vértices do grafo e legenda).
Color roleColor(Role role);
// Abreviação do papel, desenhada dentro dos vértices grandes (ex.: "L", "Dev", "QA").
const char* roleShortLabel(Role role);

}  // namespace theme

// Fontes carregadas em cada tamanho usado (carregar no tamanho certo deixa o texto nítido).
struct Fonts {
    Font small;    // 14 px, regular
    Font body;     // 16 px, regular
    Font bodyBold; // 16 px, semibold
    Font title;    // 22 px, semibold
};

// Carrega as fontes de <assetsDir>/fonts. Se não encontrar, usa a fonte padrão da raylib (sem acentos).
Fonts loadFonts(const char* assetsDir);
void unloadFonts(Fonts& fonts);

// Aplica as cores e a fonte do tema aos controles da raygui.
void applyGuiStyle(const Fonts& fonts);

// Desenha texto com a fonte e o tamanho dela (sem escala).
void drawText(const Font& font, const char* text, float x, float y, Color color);
// Largura do texto em pixels.
float measureText(const Font& font, const char* text);
