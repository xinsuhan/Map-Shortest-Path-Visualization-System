#include "renderer_phase1.h"

#include "map_transform.h"
#include "raylib.h"
#include "raymath.h"

#include <stdio.h>
#include <stdlib.h>

#define VIEW_W 1280
#define VIEW_H 820
#define TOP_BAR 72.0f
#define MARGIN 18.0f

typedef struct {
    int id;
    const char *name;
    float map_x;
    float map_y;
} CalibrationNode;

/* Debug coordinates in the 1448 x 1086 campus_map_new.png pixel space. */
static const CalibrationNode NODES[] = {
    {1, "north west road", 260, 345}, {2, "river north bend", 424, 378},
    {3, "west river road", 390, 720}, {4, "lake north west", 460, 520},
    {5, "lake north road", 660, 500}, {6, "central main road", 866, 460},
    {7, "east central road", 1100, 430}, {8, "east campus road", 1220, 388},
    {9, "lake west path", 610, 610}, {10, "east gate bridge", 804, 568},
    {11, "south lake bridge", 850, 790}, {12, "south west gate", 560, 1020}
};

static const char *map_path(void) {
    static char path[1024];
    if (FileExists("data/curved/campus_map_new.png")) return "data/curved/campus_map_new.png";
    snprintf(path, sizeof(path), "%sdata/curved/campus_map_new.png",
             GetApplicationDirectory());
    return path;
}

static MapTransform fit_transform(int width, int height) {
    float sx = (VIEW_W - 2.0f * MARGIN) / width;
    float sy = (VIEW_H - TOP_BAR - MARGIN) / height;
    float scale = sx < sy ? sx : sy;
    MapTransform result = {scale, scale, 0, 0, 0};
    result.offset_x = (VIEW_W - width * scale) * 0.5f;
    result.offset_y = TOP_BAR + (VIEW_H - TOP_BAR - height * scale) * 0.5f;
    return result;
}

static void report(const MapTransform *t, int width, int height) {
    printf("\n[Phase 1 map coordinate debug]\n");
    printf("Node count: %u\n", (unsigned)(sizeof(NODES) / sizeof(NODES[0])));
    printf("scale_x=%.6f scale_y=%.6f offset_x=%.3f offset_y=%.3f flip_y=%d\n",
           t->scale_x, t->scale_y, t->offset_x, t->offset_y, t->flip_y);
    printf("image_width=%d image_height=%d\n", width, height);
    printf("Audit: uniform scale=yes, flipped=no, origin=top-left\n");
    fflush(stdout);
}

static void draw_debug(const MapTransform *t, int width, int height) {
    size_t i;
    char text[256];
    for (i = 0; i < sizeof(NODES) / sizeof(NODES[0]); ++i) {
        Vector3 world = map_to_world(NODES[i].map_x, NODES[i].map_y, t);
        Vector2 p = {world.x, world.z};
        char id[16];
        DrawCircleV(p, 7, (Color){255, 255, 255, 235});
        DrawCircleV(p, 4.5f, (Color){226, 52, 47, 255});
        snprintf(id, sizeof(id), "N%02d", NODES[i].id);
        DrawText(id, (int)p.x + 8, (int)p.y - 7, 13, (Color){122, 20, 24, 255});
    }
    DrawRectangleRounded((Rectangle){24, 86, 430, 108}, .12f, 8,
                         (Color){251, 253, 253, 235});
    snprintf(text, sizeof(text),
             "scale_x %.6f    scale_y %.6f\noffset_x %.2f    offset_y %.2f\n"
             "flip_y %d    image %d x %d    origin (0, 0)",
             t->scale_x, t->scale_y, t->offset_x, t->offset_y,
             t->flip_y, width, height);
    DrawText(text, 40, 102, 17, (Color){40, 54, 65, 255});
}

void renderer_phase1_run(void) {
    Texture2D texture;
    MapTransform base, view;
    Vector2 drag_start = {0}, offset_start = {0};
    float zoom = 1.0f;
    int dragging = 0, debug = 1, frames = 0;
    int screenshot = getenv("MSP_PHASE1_SCREENSHOT") != NULL;
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(VIEW_W, VIEW_H, "Campus Map - Phase 1");
    SetTargetFPS(60);
    texture = LoadTexture(map_path());
    if (texture.id == 0) { CloseWindow(); return; }
    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    base = fit_transform(texture.width, texture.height);
    view = base;
    report(&view, texture.width, texture.height);

    while (!WindowShouldClose()) {
        float wheel = GetMouseWheelMove();
        if (IsKeyPressed(KEY_F3)) { debug = !debug; if (debug) report(&view, texture.width, texture.height); }
        if (IsKeyPressed(KEY_R)) { zoom = 1; view = base; }
        if (wheel != 0) {
            Vector2 mouse = GetMousePosition();
            float old = zoom;
            zoom = Clamp(zoom * (wheel > 0 ? 1.12f : 1.0f / 1.12f), .35f, 4.0f);
            view.offset_x = mouse.x - (mouse.x - view.offset_x) * zoom / old;
            view.offset_y = mouse.y - (mouse.y - view.offset_y) * zoom / old;
            view.scale_x = base.scale_x * zoom;
            view.scale_y = base.scale_y * zoom;
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            dragging = 1; drag_start = GetMousePosition();
            offset_start = (Vector2){view.offset_x, view.offset_y};
        }
        if (dragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 mouse = GetMousePosition();
            view.offset_x = offset_start.x + mouse.x - drag_start.x;
            view.offset_y = offset_start.y + mouse.y - drag_start.y;
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) dragging = 0;

        BeginDrawing();
        ClearBackground((Color){224, 236, 237, 255});
        DrawTexturePro(texture, (Rectangle){0, 0, (float)texture.width, (float)texture.height},
                       (Rectangle){view.offset_x, view.offset_y, texture.width * view.scale_x,
                                   texture.height * view.scale_y},
                       (Vector2){0, 0}, 0, WHITE);
        if (debug) draw_debug(&view, texture.width, texture.height);
        DrawRectangle(0, 0, VIEW_W, (int)TOP_BAR, (Color){246, 250, 251, 255});
        DrawText("SCU Jiang'an Campus - Phase 1 Coordinate Calibration", 24, 17, 24,
                 (Color){35, 54, 68, 255});
        DrawText(debug ? "F3  Debug nodes: ON" : "F3  Debug nodes: OFF", 980, 24, 18,
                 debug ? (Color){218, 60, 52, 255} : (Color){91, 106, 116, 255});
        DrawText("Mouse wheel: zoom   Drag: pan   R: reset", 24, VIEW_H - 28, 16,
                 (Color){45, 61, 69, 255});
        EndDrawing();
        if (screenshot && ++frames == 8) { TakeScreenshot("phase1-preview.png"); break; }
    }
    UnloadTexture(texture);
    CloseWindow();
}
