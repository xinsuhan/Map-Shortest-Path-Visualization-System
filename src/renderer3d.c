#include "renderer3d.h"

#include "astar.h"
#include "dijkstra.h"
#include "graph.h"

#include "raylib.h"
#include "raymath.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define PANEL_WIDTH 320
#define WORLD_SCALE 1.35f
#define ROAD_HEIGHT 0.035f
#define PATH_HEIGHT 0.11f

typedef struct {
    int start_id;
    int goal_id;
    int algorithm;
    int paused;
    int animation_count;
    float animation_timer;
    float animation_step;
    PathResult result;
    int has_result;
    char message[128];
} RendererState;

static Vector3 node_position(const Node *node, float y) {
    Vector3 position = {
        (float)(node->x - 9.0) * WORLD_SCALE,
        y,
        (float)(node->y - 6.5) * WORLD_SCALE
    };
    return position;
}

static Camera3D default_camera(void) {
    Camera3D camera = {0};
    camera.position = (Vector3){15.0f, 18.0f, 19.0f};
    camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 44.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    return camera;
}

static int result_contains_node(const PathResult *result, int node_id) {
    int i;
    if (result == NULL || !result->found) return 0;
    for (i = 0; i < result->path_length; ++i) {
        if (result->path[i] == node_id) return 1;
    }
    return 0;
}

static int result_contains_edge(const PathResult *result, int from_id, int to_id) {
    int i;
    if (result == NULL || !result->found) return 0;
    for (i = 0; i + 1 < result->path_length; ++i) {
        if ((result->path[i] == from_id && result->path[i + 1] == to_id) ||
            (result->path[i] == to_id && result->path[i + 1] == from_id)) {
            return 1;
        }
    }
    return 0;
}

static int visited_contains(const RendererState *state, int node_id) {
    int i;
    if (!state->has_result) return 0;
    for (i = 0; i < state->animation_count && i < state->result.visited_count; ++i) {
        if (state->result.visited_order[i] == node_id) return 1;
    }
    return 0;
}

static Color node_color(const Node *node, const RendererState *state) {
    if (node->id == state->start_id) return (Color){66, 200, 112, 255};
    if (node->id == state->goal_id) return (Color){235, 82, 82, 255};
    if (state->has_result && state->animation_count >= state->result.visited_count &&
        result_contains_node(&state->result, node->id)) {
        return (Color){255, 194, 54, 255};
    }
    if (visited_contains(state, node->id)) return (Color){86, 155, 230, 255};
    switch (node->type) {
        case NODE_LAKE: return (Color){76, 158, 211, 220};
        case NODE_SQUARE: return (Color){205, 190, 150, 255};
        case NODE_GATE: return (Color){183, 126, 86, 255};
        case NODE_JUNCTION: return (Color){115, 125, 132, 255};
        case NODE_LANDMARK:
        default: return (Color){145, 164, 172, 255};
    }
}

static void draw_gate(const Node *node, Color color) {
    Vector3 center = node_position(node, 0.0f);
    float width = fmaxf(node->width * WORLD_SCALE, 0.8f);
    float height = fmaxf(node->height * WORLD_SCALE, 0.32f);
    float depth = fmaxf(node->depth * WORLD_SCALE, 0.35f);
    Vector3 left = {center.x - width * 0.42f, height * 0.5f, center.z};
    Vector3 right = {center.x + width * 0.42f, height * 0.5f, center.z};
    Vector3 top = {center.x, height, center.z};
    DrawCube(left, width * 0.16f, height, depth, color);
    DrawCube(right, width * 0.16f, height, depth, color);
    DrawCube(top, width, height * 0.20f, depth, color);
}

static void draw_node(const Node *node, const RendererState *state) {
    Color color = node_color(node, state);
    Vector3 center = node_position(node, 0.0f);
    float width = fmaxf(node->width * WORLD_SCALE, 0.18f);
    float depth = fmaxf(node->depth * WORLD_SCALE, 0.18f);
    float height = fmaxf(node->height * WORLD_SCALE, 0.04f);

    if (node->type == NODE_JUNCTION) {
        DrawSphere((Vector3){center.x, 0.075f, center.z}, 0.075f, color);
    } else if (node->type == NODE_GATE) {
        draw_gate(node, color);
    } else {
        center.y = height * 0.5f;
        DrawCube(center, width, height, depth, color);
        if (node->type == NODE_LANDMARK) {
            DrawCubeWires(center, width, height, depth, Fade(DARKGRAY, 0.55f));
        }
    }
}

static void draw_roads(const Graph *graph, const RendererState *state) {
    int i;
    int show_path = state->has_result &&
                    state->animation_count >= state->result.visited_count;
    for (i = 0; i < graph->edge_count; ++i) {
        const Edge *edge = &graph->edges[i];
        const Node *from = graph_get_node(graph, edge->from_id);
        const Node *to = graph_get_node(graph, edge->to_id);
        if (from == NULL || to == NULL) continue;
        DrawCylinderEx(node_position(from, ROAD_HEIGHT), node_position(to, ROAD_HEIGHT),
                       0.035f, 0.035f, 6, (Color){177, 183, 186, 185});
        if (show_path && result_contains_edge(&state->result, edge->from_id, edge->to_id)) {
            DrawCylinderEx(node_position(from, PATH_HEIGHT), node_position(to, PATH_HEIGHT),
                           0.11f, 0.11f, 10, (Color){255, 190, 45, 255});
        }
    }
}

static int pick_node(const Graph *graph, Ray ray) {
    int i;
    int picked = -1;
    float nearest = 1.0e30f;
    for (i = 0; i < graph->node_count; ++i) {
        const Node *node = &graph->nodes[i];
        Vector3 center;
        Vector3 half;
        BoundingBox box;
        RayCollision hit;
        if (node->type == NODE_JUNCTION) continue;
        center = node_position(node, fmaxf(node->height * WORLD_SCALE, 0.4f) * 0.5f);
        half = (Vector3){fmaxf(node->width * WORLD_SCALE * 0.6f, 0.45f),
                         fmaxf(node->height * WORLD_SCALE, 0.45f),
                         fmaxf(node->depth * WORLD_SCALE * 0.6f, 0.45f)};
        box.min = Vector3Subtract(center, half);
        box.max = Vector3Add(center, half);
        hit = GetRayCollisionBox(ray, box);
        if (hit.hit && hit.distance < nearest) {
            nearest = hit.distance;
            picked = node->id;
        }
    }
    return picked;
}

static void run_search(const Graph *graph, RendererState *state) {
    int status;
    if (state->start_id < 0 || state->goal_id < 0) {
        snprintf(state->message, sizeof(state->message), "Select start and goal first");
        return;
    }
    if (state->start_id == state->goal_id) {
        snprintf(state->message, sizeof(state->message), "Start and goal must differ");
        return;
    }
    status = state->algorithm == 1
                 ? dijkstra_find_path(graph, state->start_id, state->goal_id, &state->result)
                 : astar_find_path(graph, state->start_id, state->goal_id, &state->result);
    state->has_result = status == MSP_OK;
    state->animation_count = 0;
    state->animation_timer = 0.0f;
    state->paused = 0;
    if (status == MSP_OK) {
        snprintf(state->message, sizeof(state->message), "Search running...");
    } else if (status == MSP_ERROR_NO_PATH) {
        snprintf(state->message, sizeof(state->message), "No path found");
    } else {
        snprintf(state->message, sizeof(state->message), "Search failed: %d", status);
    }
}

static void update_animation(RendererState *state) {
    if (!state->has_result || state->paused ||
        state->animation_count >= state->result.visited_count) return;
    state->animation_timer += GetFrameTime();
    if (state->animation_timer >= state->animation_step) {
        state->animation_timer = 0.0f;
        state->animation_count++;
        if (state->animation_count >= state->result.visited_count) {
            snprintf(state->message, sizeof(state->message), "Search complete");
        }
    }
}

static void draw_labels(const Graph *graph, const Camera3D *camera,
                        const RendererState *state, int view_width) {
    int i;
    for (i = 0; i < graph->node_count; ++i) {
        const Node *node = &graph->nodes[i];
        Vector2 screen;
        const char *label;
        if (node->type == NODE_JUNCTION && !result_contains_node(&state->result, node->id)) {
            continue;
        }
        screen = GetWorldToScreen(node_position(node, node->height * WORLD_SCALE + 0.18f), *camera);
        if (screen.x < 0 || screen.x >= view_width || screen.y < 0 || screen.y >= GetScreenHeight()) {
            continue;
        }
        label = (node->id == state->start_id || node->id == state->goal_id ||
                 result_contains_node(&state->result, node->id))
                    ? node->name
                    : TextFormat("%d", node->id);
        DrawText(label, (int)screen.x - MeasureText(label, 14) / 2,
                 (int)screen.y - 8, 14, (Color){35, 42, 45, 255});
    }
}

static void draw_panel(const Graph *graph, const RendererState *state) {
    int x = GetScreenWidth() - PANEL_WIDTH;
    const Node *start = graph_get_node(graph, state->start_id);
    const Node *goal = graph_get_node(graph, state->goal_id);
    int y = 24;
    DrawRectangle(x, 0, PANEL_WIDTH, GetScreenHeight(), (Color){245, 247, 246, 248});
    DrawLine(x, 0, x, GetScreenHeight(), (Color){190, 198, 198, 255});
    DrawText("SCU PATH 3D", x + 20, y, 24, (Color){31, 67, 63, 255});
    y += 46;
    DrawText(TextFormat("Start: %s", start ? start->name : "not selected"), x + 20, y, 16, DARKGREEN);
    y += 26;
    DrawText(TextFormat("Goal:  %s", goal ? goal->name : "not selected"), x + 20, y, 16, MAROON);
    y += 34;
    DrawText(TextFormat("Algorithm: %s", state->algorithm == 1 ? "Dijkstra" : "A*"), x + 20, y, 18, DARKGRAY);
    y += 28;
    if (state->has_result) {
        DrawText(TextFormat("Distance: %.2f", state->result.total_distance), x + 20, y, 18, DARKGRAY);
        y += 26;
        DrawText(TextFormat("Visited: %d / %d", state->animation_count,
                            state->result.visited_count), x + 20, y, 18, DARKGRAY);
        y += 34;
    }
    DrawText(state->message, x + 20, y, 16, (Color){60, 90, 88, 255});
    y = GetScreenHeight() - 225;
    DrawText("Controls", x + 20, y, 20, (Color){31, 67, 63, 255});
    y += 32;
    DrawText("Left click: start", x + 20, y, 15, DARKGRAY); y += 23;
    DrawText("Right click: goal", x + 20, y, 15, DARKGRAY); y += 23;
    DrawText("D / A: algorithm", x + 20, y, 15, DARKGRAY); y += 23;
    DrawText("Enter: search   Space: pause", x + 20, y, 15, DARKGRAY); y += 23;
    DrawText("1 / 2 / 3: animation speed", x + 20, y, 15, DARKGRAY); y += 23;
    DrawText("WASD + mouse wheel: camera", x + 20, y, 15, DARKGRAY); y += 23;
    DrawText("Home: camera   R: reset", x + 20, y, 15, DARKGRAY); y += 23;
    DrawText("Esc: return to console", x + 20, y, 15, DARKGRAY);
}

void renderer3d_run(const Graph *graph) {
    Camera3D camera = default_camera();
    RendererState state = {0};
    int screenshot_mode = getenv("MSP_3D_SCREENSHOT") != NULL;
    int frame_count = 0;
    state.start_id = -1;
    state.goal_id = -1;
    state.algorithm = 1;
    state.animation_step = 0.22f;
    snprintf(state.message, sizeof(state.message), "Select two landmarks");

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "SCU Jiang'an Campus - 3D Shortest Path");
    SetTargetFPS(60);
    if (screenshot_mode) {
        state.start_id = 0;
        state.goal_id = 7;
        run_search(graph, &state);
        state.animation_count = state.result.visited_count;
    }

    while (!WindowShouldClose()) {
        int view_width = GetScreenWidth() - PANEL_WIDTH;
        int picked;
        if (IsKeyPressed(KEY_HOME)) camera = default_camera();
        if (IsKeyPressed(KEY_D)) state.algorithm = 1;
        if (IsKeyPressed(KEY_A)) state.algorithm = 2;
        if (IsKeyPressed(KEY_ONE)) state.animation_step = 0.55f;
        if (IsKeyPressed(KEY_TWO)) state.animation_step = 0.22f;
        if (IsKeyPressed(KEY_THREE)) state.animation_step = 0.07f;
        if (IsKeyPressed(KEY_SPACE)) state.paused = !state.paused;
        if (IsKeyPressed(KEY_ENTER)) run_search(graph, &state);
        if (IsKeyPressed(KEY_R)) {
            state.start_id = -1;
            state.goal_id = -1;
            state.has_result = 0;
            state.animation_count = 0;
            snprintf(state.message, sizeof(state.message), "Selection reset");
        }

        if (GetMouseX() < view_width &&
            (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
             IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))) {
            picked = pick_node(graph, GetMouseRay(GetMousePosition(), camera));
            if (picked >= 0) {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) state.start_id = picked;
                if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) state.goal_id = picked;
                state.has_result = 0;
                snprintf(state.message, sizeof(state.message), "Press Enter to search");
            }
        }

        UpdateCamera(&camera, CAMERA_FREE);
        update_animation(&state);

        BeginDrawing();
        ClearBackground((Color){226, 234, 229, 255});
        BeginMode3D(camera);
        DrawPlane((Vector3){0.0f, -0.015f, 0.0f}, (Vector2){23.0f, 18.0f},
                  (Color){214, 222, 211, 255});
        draw_roads(graph, &state);
        {
            int i;
            for (i = 0; i < graph->node_count; ++i) draw_node(&graph->nodes[i], &state);
        }
        EndMode3D();
        draw_labels(graph, &camera, &state, view_width);
        draw_panel(graph, &state);
        EndDrawing();
        frame_count++;
        if (screenshot_mode && frame_count == 12) {
            TakeScreenshot("3d-preview.png");
            break;
        }
    }
    CloseWindow();
}
