#include "theme.hpp"

#include <string>
#include <vector>

#include "raygui.h"

namespace theme {

Color roleColor(Role role) {
    switch (role) {
        case Role::Leader: return {232, 89, 12, 255};     // laranja
        case Role::Dev: return {25, 113, 194, 255};       // azul
        case Role::Designer: return {156, 54, 181, 255};  // roxo
        case Role::Architect: return {43, 138, 62, 255};  // verde
        case Role::QA: return {214, 51, 108, 255};        // framboesa
        case Role::DevOps: return {73, 80, 87, 255};      // grafite
        case Role::Data: return {12, 133, 153, 255};      // petróleo
    }
    return GRAY;
}

const char* roleShortLabel(Role role) {
    switch (role) {
        case Role::Leader: return "L";
        case Role::Dev: return "Dev";
        case Role::Designer: return "Ds";
        case Role::Architect: return "Arq";
        case Role::QA: return "QA";
        case Role::DevOps: return "Ops";
        case Role::Data: return "Da";
    }
    return "?";
}

}  // namespace theme

namespace {

// ASCII, Latin-1 (acentos do português) e alguns símbolos usados na interface.
std::vector<int> uiCodepoints() {
    std::vector<int> codepoints;
    for (int c = 0x20; c <= 0x7E; ++c) codepoints.push_back(c);
    for (int c = 0xA0; c <= 0xFF; ++c) codepoints.push_back(c);
    for (int c : {0x2013, 0x2014, 0x2022, 0x2026, 0x2190, 0x2192, 0x2194, 0x25B6, 0x25C0, 0x2713, 0x2717}) {
        codepoints.push_back(c);
    }
    return codepoints;
}

Font loadFont(const char* assetsDir, const char* fileName, int size) {
    const std::string path = std::string(assetsDir) + "/fonts/" + fileName;
    if (!FileExists(path.c_str())) return GetFontDefault();
    std::vector<int> codepoints = uiCodepoints();
    Font font = LoadFontEx(path.c_str(), size, codepoints.data(), static_cast<int>(codepoints.size()));
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    return font;
}

void unloadIfCustom(Font& font) {
    if (font.texture.id != GetFontDefault().texture.id) UnloadFont(font);
}

}  // namespace

Fonts loadFonts(const char* assetsDir) {
    Fonts fonts;
    fonts.small = loadFont(assetsDir, "Inter-Regular.ttf", 14);
    fonts.body = loadFont(assetsDir, "Inter-Regular.ttf", 16);
    fonts.bodyBold = loadFont(assetsDir, "Inter-SemiBold.ttf", 16);
    fonts.title = loadFont(assetsDir, "Inter-SemiBold.ttf", 22);
    return fonts;
}

void unloadFonts(Fonts& fonts) {
    unloadIfCustom(fonts.small);
    unloadIfCustom(fonts.body);
    unloadIfCustom(fonts.bodyBold);
    unloadIfCustom(fonts.title);
}

void applyGuiStyle(const Fonts& fonts) {
    GuiLoadStyleDefault();
    GuiSetFont(fonts.body);
    GuiSetStyle(DEFAULT, TEXT_SIZE, fonts.body.baseSize);
    GuiSetStyle(DEFAULT, TEXT_SPACING, 0);

    auto set = [](int property, Color color) { GuiSetStyle(DEFAULT, property, ColorToInt(color)); };
    set(BORDER_COLOR_NORMAL, theme::kBorder);
    set(BASE_COLOR_NORMAL, theme::kPanel);
    set(TEXT_COLOR_NORMAL, theme::kTextPrimary);
    set(BORDER_COLOR_FOCUSED, theme::kAccent);
    set(BASE_COLOR_FOCUSED, theme::kCardSelected);
    set(TEXT_COLOR_FOCUSED, theme::kAccent);
    set(BORDER_COLOR_PRESSED, theme::kAccent);
    set(BASE_COLOR_PRESSED, Color{213, 226, 251, 255});
    set(TEXT_COLOR_PRESSED, theme::kAccent);
    set(BORDER_COLOR_DISABLED, theme::kBorder);
    set(BASE_COLOR_DISABLED, Color{241, 243, 245, 255});
    set(TEXT_COLOR_DISABLED, theme::kTextMuted);
    set(LINE_COLOR, theme::kBorder);
    set(BACKGROUND_COLOR, theme::kPanel);
}

void drawText(const Font& font, const char* text, float x, float y, Color color) {
    DrawTextEx(font, text, {x, y}, static_cast<float>(font.baseSize), 0.0f, color);
}

float measureText(const Font& font, const char* text) {
    return MeasureTextEx(font, text, static_cast<float>(font.baseSize), 0.0f).x;
}
