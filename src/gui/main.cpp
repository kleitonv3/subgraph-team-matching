// Ponto de entrada da interface gráfica.

#include "app.hpp"
#include "raylib.h"

int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "Subgraph Team Matching");
    SetTargetFPS(60);

    App app;
    app.init(GetApplicationDirectory());
    while (!WindowShouldClose()) app.frame();

    app.shutdown();
    CloseWindow();
    return 0;
}
