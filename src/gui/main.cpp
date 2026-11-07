// Ponto de entrada da interface gráfica.
//
// Fase 1: só abre a janela, carrega a fonte (com acentos) e desenha um texto de teste e um botão da
// raygui, para confirmar que o build e as bibliotecas estão funcionando.

#include <string>
#include <vector>

#include "raylib.h"
#include "raygui.h"

namespace {

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;

constexpr int kTitleSize = 40;
constexpr int kBodySize = 22;

const Color kBackground = {245, 246, 250, 255};
const Color kTextPrimary = {30, 34, 45, 255};
const Color kTextSecondary = {90, 98, 115, 255};

// Caracteres que a fonte precisa ter: ASCII, Latin-1 (acentos do português) e alguns símbolos
// usados na interface (travessão, reticências, setas, play/pause).
std::vector<int> uiCodepoints() {
    std::vector<int> codepoints;
    for (int c = 0x20; c <= 0x7E; ++c) codepoints.push_back(c);
    for (int c = 0xA0; c <= 0xFF; ++c) codepoints.push_back(c);
    for (int c : {0x2013, 0x2014, 0x2026, 0x2190, 0x2192, 0x2194, 0x25B6, 0x25C0, 0x2713, 0x2717}) {
        codepoints.push_back(c);
    }
    return codepoints;
}

// Procura a fonte ao lado do executável (bin/../assets) e, se não achar, a partir da pasta atual.
Font loadUiFont(const char* fileName, int size) {
    std::string path = std::string(GetApplicationDirectory()) + "../assets/fonts/" + fileName;
    if (!FileExists(path.c_str())) path = std::string("assets/fonts/") + fileName;

    std::vector<int> codepoints = uiCodepoints();
    Font font = LoadFontEx(path.c_str(), size, codepoints.data(), static_cast<int>(codepoints.size()));
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    return font;
}

}  // namespace

int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(kWindowWidth, kWindowHeight, "Subgraph Team Matching");
    SetTargetFPS(60);

    Font titleFont = loadUiFont("Inter-SemiBold.ttf", kTitleSize);
    Font bodyFont = loadUiFont("Inter-Regular.ttf", kBodySize);

    GuiSetFont(bodyFont);
    GuiSetStyle(DEFAULT, TEXT_SIZE, kBodySize);

    bool quit = false;
    while (!quit && !WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(kBackground);

        DrawTextEx(titleFont, "Subgraph Team Matching", {60, 60}, kTitleSize, 0, kTextPrimary);
        DrawTextEx(bodyFont, "Formação de equipes por isomorfismo de subgrafos", {60, 115}, kBodySize, 0,
                   kTextSecondary);
        DrawTextEx(bodyFont, "Fase 1 — janela e fonte funcionando. Acentuação: ã é ç ô í ú À → ▶", {60, 170},
                   kBodySize, 0, kTextPrimary);

        if (GuiButton({60, 230, 160, 44}, "Sair")) quit = true;

        EndDrawing();
    }

    UnloadFont(titleFont);
    UnloadFont(bodyFont);
    CloseWindow();
    return 0;
}
