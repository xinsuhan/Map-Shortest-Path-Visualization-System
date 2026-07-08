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
#define RENDER_SCALE_X 1.80f
#define RENDER_SCALE_Z 1.45f
#define BUILDING_SCALE 0.82f
#define HEIGHT_SCALE 0.55f
#define ROAD_HEIGHT 0.025f
#define PATH_HEIGHT 0.055f
#define ROAD_WIDTH 0.105f
#define PATH_WIDTH 0.32f

typedef enum {
    VIEW_OVERVIEW = 0,
    VIEW_NAVIGATION,
    VIEW_PATH
} CameraView;

typedef struct {
    float center_x;
    float center_z;
    float min_x;
    float max_x;
    float min_z;
    float max_z;
    float overview_size;
} SceneLayout;

typedef struct {
    int start_id;
    int goal_id;
    int hover_id;
    int algorithm;
    int paused;
    int animation_count;
    float animation_timer;
    float animation_step;
    double search_ms;
    PathResult result;
    int has_result;
    CameraView camera_view;
    char message[128];
} RendererState;

static float clamp_float(float value, float minimum, float maximum) {
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

static SceneLayout build_scene_layout(const Graph *graph) {
    SceneLayout layout = {0};
    double min_x = graph->nodes[0].x;
    double max_x = graph->nodes[0].x;
    double min_y = graph->nodes[0].y;
    double max_y = graph->nodes[0].y;
    int i;

    for (i = 1; i < graph->node_count; ++i) {
        if (graph->nodes[i].x < min_x) min_x = graph->nodes[i].x;
        if (graph->nodes[i].x > max_x) max_x = graph->nodes[i].x;
        if (graph->nodes[i].y < min_y) min_y = graph->nodes[i].y;
        if (graph->nodes[i].y > max_y) max_y = graph->nodes[i].y;
    }

    layout.center_x = (float)((min_x + max_x) * 0.5);
    layout.center_z = (float)((min_y + max_y) * 0.5);
    layout.min_x = (float)(min_x - layout.center_x) * RENDER_SCALE_X;
    layout.max_x = (float)(max_x - layout.center_x) * RENDER_SCALE_X;
    layout.min_z = (float)(min_y - layout.center_z) * RENDER_SCALE_Z;
    layout.max_z = (float)(max_y - layout.center_z) * RENDER_SCALE_Z;
    layout.overview_size =
        fmaxf(layout.max_z - layout.min_z, (layout.max_x - layout.min_x) / 1.30f) * 1.28f;
    return layout;
}

static Vector3 node_position(const Node *node, const SceneLayout *layout, float y) {
    Vector3 position = {
        (float)(node->x - layout->center_x) * RENDER_SCALE_X,
        y,
        (float)(node->y - layout->center_z) * RENDER_SCALE_Z
    };
    return position;
}

static float node_render_height(const Node *node) {
    switch (node->type) {
        case NODE_LAKE: return 0.025f;
        case NODE_SQUARE: return 0.035f;
        case NODE_JUNCTION: return 0.025f;
        case NODE_GATE:
            return clamp_float(node->height * HEIGHT_SCALE, 0.12f, 0.26f);
        case NODE_LANDMARK:
        default:
            return clamp_float(node->height * HEIGHT_SCALE, 0.10f, 0.35f);
    }
}

static float node_render_width(const Node *node) {
    return fmaxf(node->width * RENDER_SCALE_X * BUILDING_SCALE, 0.22f);
}

static float node_render_depth(const Node *node) {
    return fmaxf(node->depth * RENDER_SCALE_Z * BUILDING_SCALE, 0.22f);
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

static int current_node_id(const RendererState *state) {
    if (!state->has_result || state->animation_count <= 0 ||
        state->animation_count > state->result.visited_count) {
        return -1;
    }
    return state->result.visited_order[state->animation_count - 1];
}

static Camera3D camera_for_view(const Graph *graph, const SceneLayout *layout,
                                const RendererState *state, CameraView view) {
    Camera3D camera = {0};
    Vector3 target = {0.0f, 0.0f, 0.0f};
    float size = layout->overview_size;

    if (view == VIEW_PATH && state->has_result && state->result.found) {
        float min_x = 1.0e30f;
        float max_x = -1.0e30f;
        float min_z = 1.0e30f;
        float max_z = -1.0e30f;
        int i;
        for (i = 0; i < state->result.path_length; ++i) {
            const Node *node = graph_get_node(graph, state->result.path[i]);
            Vector3 position;
            if (node == NULL) continue;
            position = node_position(node, layout, 0.0f);
            if (position.x < min_x) min_x = position.x;
            if (position.x > max_x) max_x = position.x;
            if (position.z < min_z) min_z = position.z;
            if (position.z > max_z) max_z = position.z;
        }
        if (min_x <= max_x && min_z <= max_z) {
            target.x = (min_x + max_x) * 0.5f;
            target.z = (min_z + max_z) * 0.5f;
            size = fmaxf(max_z - min_z, (max_x - min_x) / 1.30f) * 1.45f;
            size = clamp_float(size, 7.0f, layout->overview_size);
        }
    }

    camera.target = target;
    if (view == VIEW_NAVIGATION) {
        camera.position = (Vector3){target.x, size * 1.22f, target.z + size * 0.28f};
        camera.fovy = size * 0.92f;
    } else {
        camera.position = (Vector3){target.x + size * 0.12f, size * 1.02f,
                                    target.z + size * 0.48f};
        camera.fovy = size;
    }
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.projection = CAMERA_ORTHOGRAPHIC;
    return camera;
}

static Color node_color(const Node *node, const RendererState *state) {
    if (node->id == state->start_id) return (Color){38, 166, 91, 255};
    if (node->id == state->goal_id) return (Color){218, 63, 63, 255};
    if (node->id == current_node_id(state)) return (Color){72, 126, 214, 255};
    if (state->has_result && state->animation_count >= state->result.visited_count &&
        result_contains_node(&state->result, node->id)) {
        return (Color){248, 177, 42, 255};
    }
    if (visited_contains(state, node->id)) return (Color){111, 164, 211, 255};
    switch (node->type) {
        case NODE_LAKE: return (Color){76, 157, 205, 235};
        case NODE_SQUARE: return (Color){196, 183, 143, 255};
        case NODE_GATE: return (Color){174, 112, 72, 255};
        case NODE_JUNCTION: return (Color){112, 124, 128, 210};
        case NODE_LANDMARK:
        default: return (Color){130, 153, 158, 255};
    }
}

static void draw_gate(const Node *node, const SceneLayout *layout, Color color) {
    Vector3 center = node_position(node, layout, 0.0f);
    float width = fmaxf(node_render_width(node), 0.85f);
    float height = node_render_height(node);
    float depth = fmaxf(node_render_depth(node), 0.35f);
    Vector3 left = {center.x - width * 0.40f, height * 0.5f, center.z};
    Vector3 right = {center.x + width * 0.40f, height * 0.5f, center.z};
    Vector3 top = {center.x, height, center.z};
    DrawCube(left, width * 0.16f, height, depth, color);
    DrawCube(right, width * 0.16f, height, depth, color);
    DrawCube(top, width, height * 0.22f, depth, color);
}

static void draw_node(const Node *node, const SceneLayout *layout,
                      const RendererState *state) {
    Color color = node_color(node, state);
    Vector3 center = node_position(node, layout, 0.0f);
    float width = node_render_width(node);
    float depth = node_render_depth(node);
    float height = node_render_height(node);

    if (node->type == NODE_JUNCTION) {
        DrawCylinder((Vector3){center.x, height * 0.5f, center.z},
                     0.105f, 0.105f, height, 16, color);
    } else if (node->type == NODE_GATE) {
        draw_gate(node, layout, color);
    } else {
        center.y = height * 0.5f;
        DrawCube(center, width, height, depth, color);
        if (node->type == NODE_LAKE) {
            DrawCubeWires(center, width, height, depth, Fade((Color){39, 105, 151, 255}, 0.8f));
        } else if (node->type == NODE_SQUARE) {
            DrawCubeWires(center, width, height, depth, Fade((Color){126, 112, 79, 255}, 0.55f));
        } else {
            DrawCubeWires(center, width, height, depth, Fade((Color){55, 70, 73, 255}, 0.5f));
        }
    }
}

static void draw_band(Vector3 from, Vector3 to, float width, Color color) {
    Vector3 points[4];
    float dx = to.x - from.x;
    float dz = to.z - from.z;
    float length = sqrtf(dx * dx + dz * dz);
    float offset_x;
    float offset_z;
    if (length <= 0.0001f) return;
    offset_x = -dz / length * width * 0.5f;
    offset_z = dx / length * width * 0.5f;
    points[0] = (Vector3){from.x - offset_x, from.y, from.z - offset_z};
    points[1] = (Vector3){from.x + offset_x, from.y, from.z + offset_z};
    points[2] = (Vector3){to.x - offset_x, to.y, to.z - offset_z};
    points[3] = (Vector3){to.x + offset_x, to.y, to.z + offset_z};
    DrawTriangleStrip3D(points, 4, color);
}

static void draw_roads(const Graph *graph, const SceneLayout *layout,
                       const RendererState *state) {
    int i;
    int show_path = state->has_result &&
                    state->animation_count >= state->result.visited_count;

    for (i = 0; i < graph->edge_count; ++i) {
        const Edge *edge = &graph->edges[i];
        const Node *from = graph_get_node(graph, edge->from_id);
        const Node *to = graph_get_node(graph, edge->to_id);
        if (from == NULL || to == NULL) continue;
        draw_band(node_position(from, layout, ROAD_HEIGHT),
                  node_position(to, layout, ROAD_HEIGHT), ROAD_WIDTH,
                  (Color){137, 151, 154, 175});
    }
    if (!show_path) return;
    for (i = 0; i < graph->edge_count; ++i) {
        const Edge *edge = &graph->edges[i];
        const Node *from;
        const Node *to;
        if (!result_contains_edge(&state->result, edge->from_id, edge->to_id)) continue;
        from = graph_get_node(graph, edge->from_id);
        to = graph_get_node(graph, edge->to_id);
        if (from == NULL || to == NULL) continue;
        draw_band(node_position(from, layout, PATH_HEIGHT),
                  node_position(to, layout, PATH_HEIGHT), PATH_WIDTH,
                  (Color){250, 174, 35, 255});
    }
}

static int pick_node(const Graph *graph, const SceneLayout *layout, Ray ray) {
    int i;
    int picked = -1;
    float nearest = 1.0e30f;
    for (i = 0; i < graph->node_count; ++i) {
        const Node *node = &graph->nodes[i];
        Vector3 center;
        Vector3 half;
        BoundingBox box;
        RayCollision hit;
        float height;
        if (node->type == NODE_JUNCTION) continue;
        height = node_render_height(node);
        center = node_position(node, layout, height * 0.5f);
        half = (Vector3){fmaxf(node_render_width(node) * 0.55f, 0.30f),
                         fmaxf(height, 0.18f),
                         fmaxf(node_render_depth(node) * 0.55f, 0.30f)};
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
    double started_at;
    if (state->start_id < 0 || state->goal_id < 0) {
        snprintf(state->message, sizeof(state->message), "Select start and goal first");
        return;
    }
    if (state->start_id == state->goal_id) {
        snprintf(state->message, sizeof(state->message), "Start and goal must differ");
        return;
    }
    started_at = GetTime();
    status = state->algorithm == 1
                 ? dijkstra_find_path(graph, state->start_id, state->goal_id, &state->result)
                 : astar_find_path(graph, state->start_id, state->goal_id, &state->result);
    state->search_ms = (GetTime() - started_at) * 1000.0;
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

static int is_core_label(const Node *node) {
    return node->type == NODE_GATE || node->type == NODE_LAKE ||
           node->type == NODE_SQUARE || node->id == 7;
}

static int label_is_visible(const Node *node, const Camera3D *camera,
                            const SceneLayout *layout, const RendererState *state) {
    if (node->id == state->start_id || node->id == state->goal_id ||
        node->id == state->hover_id || node->id == current_node_id(state) ||
        result_contains_node(&state->result, node->id)) {
        return 1;
    }
    if (node->type == NODE_JUNCTION) return 0;
    if (is_core_label(node)) return 1;
    return camera->fovy < layout->overview_size * 0.62f;
}

static void draw_labels(const Graph *graph, const SceneLayout *layout,
                        const Camera3D *camera, const RendererState *state,
                        int view_width) {
    Rectangle occupied[MSP_MAX_NODES];
    int occupied_count = 0;
    int pass;
    int i;

    for (pass = 0; pass < 2; ++pass) {
        for (i = 0; i < graph->node_count; ++i) {
            const Node *node = &graph->nodes[i];
            Vector2 screen;
            Rectangle bounds;
            const char *label;
            int priority = node->id == state->start_id || node->id == state->goal_id ||
                           node->id == state->hover_id ||
                           node->id == current_node_id(state) ||
                           result_contains_node(&state->result, node->id);
            int collision = 0;
            int j;
            int font_size = priority ? 15 : 13;

            if (priority != (pass == 0) ||
                !label_is_visible(node, camera, layout, state)) {
                continue;
            }
            screen = GetWorldToScreen(
                node_position(node, layout, node_render_height(node) + 0.16f), *camera);
            if (screen.x < 4 || screen.x >= view_width - 4 ||
                screen.y < 4 || screen.y >= GetScreenHeight() - 4) {
                continue;
            }
            if (priority && node->type == NODE_JUNCTION) {
                label = TextFormat("J%d", node->id);
            } else {
                label = priority ? node->name : TextFormat("%d", node->id);
            }
            bounds = (Rectangle){screen.x - MeasureText(label, font_size) * 0.5f - 4.0f,
                                 screen.y - font_size - 5.0f,
                                 (float)MeasureText(label, font_size) + 8.0f,
                                 (float)font_size + 6.0f};
            for (j = 0; j < occupied_count; ++j) {
                if (CheckCollisionRecs(bounds, occupied[j])) {
                    collision = 1;
                    break;
                }
            }
            if (collision && !priority) continue;
            DrawRectangleRec(bounds, priority ? (Color){249, 250, 246, 225}
                                                : (Color){239, 243, 239, 205});
            DrawRectangleLinesEx(bounds, 1.0f, Fade((Color){74, 89, 88, 255}, 0.35f));
            DrawText(label, (int)bounds.x + 4, (int)bounds.y + 3, font_size,
                     (Color){35, 45, 45, 255});
            if (occupied_count < MSP_MAX_NODES) occupied[occupied_count++] = bounds;
        }
    }
}

static Rectangle camera_button(int index) {
    float x = (float)GetScreenWidth() - PANEL_WIDTH + 18.0f + index * 94.0f;
    return (Rectangle){x, 294.0f, 86.0f, 30.0f};
}

static void draw_camera_button(int index, const char *text, int active) {
    Rectangle button = camera_button(index);
    DrawRectangleRec(button, active ? (Color){51, 106, 97, 255}
                                    : (Color){224, 231, 227, 255});
    DrawRectangleLinesEx(button, 1.0f, (Color){139, 155, 151, 255});
    DrawText(text, (int)(button.x + button.width * 0.5f -
                        MeasureText(text, 13) * 0.5f),
             (int)button.y + 8, 13, active ? RAYWHITE : (Color){55, 68, 66, 255});
}

static void draw_panel(const Graph *graph, const RendererState *state) {
    int x = GetScreenWidth() - PANEL_WIDTH;
    const Node *start = graph_get_node(graph, state->start_id);
    const Node *goal = graph_get_node(graph, state->goal_id);
    int y = 22;
    DrawRectangle(x, 0, PANEL_WIDTH, GetScreenHeight(), (Color){245, 247, 244, 250});
    DrawLine(x, 0, x, GetScreenHeight(), (Color){179, 190, 187, 255});
    DrawText("SCU CAMPUS NAV", x + 18, y, 23, (Color){31, 75, 68, 255});
    y += 44;
    DrawText(TextFormat("Start: %s", start ? start->name : "not selected"),
             x + 18, y, 15, DARKGREEN);
    y += 25;
    DrawText(TextFormat("Goal:  %s", goal ? goal->name : "not selected"),
             x + 18, y, 15, MAROON);
    y += 32;
    DrawText(TextFormat("Algorithm: %s", state->algorithm == 1 ? "Dijkstra" : "A*"),
             x + 18, y, 17, DARKGRAY);
    y += 26;
    if (state->has_result) {
        int junctions = 0;
        int i;
        for (i = 0; i < state->result.path_length; ++i) {
            const Node *node = graph_get_node(graph, state->result.path[i]);
            if (node != NULL && node->type == NODE_JUNCTION) junctions++;
        }
        DrawText(TextFormat("Distance: %.2f", state->result.total_distance),
                 x + 18, y, 17, DARKGRAY);
        y += 23;
        DrawText(TextFormat("Visited: %d / %d", state->animation_count,
                            state->result.visited_count), x + 18, y, 15, DARKGRAY);
        y += 22;
        DrawText(TextFormat("Route: %d nodes, %d junctions",
                            state->result.path_length, junctions),
                 x + 18, y, 15, DARKGRAY);
        y += 22;
        DrawText(TextFormat("Search: %.3f ms", state->search_ms),
                 x + 18, y, 15, DARKGRAY);
        y += 26;
    }
    DrawText(state->message, x + 18, y, 15, (Color){56, 88, 82, 255});

    DrawText("Views", x + 18, 270, 16, (Color){31, 75, 68, 255});
    draw_camera_button(0, "Overview", state->camera_view == VIEW_OVERVIEW);
    draw_camera_button(1, "Navigate", state->camera_view == VIEW_NAVIGATION);
    draw_camera_button(2, "Path", state->camera_view == VIEW_PATH);

    y = GetScreenHeight() - 252;
    DrawText("Controls", x + 18, y, 19, (Color){31, 75, 68, 255});
    y += 31;
    DrawText("Left click: start", x + 18, y, 14, DARKGRAY); y += 22;
    DrawText("Right click: goal", x + 18, y, 14, DARKGRAY); y += 22;
    DrawText("D / A: algorithm", x + 18, y, 14, DARKGRAY); y += 22;
    DrawText("Enter: search   Space: pause", x + 18, y, 14, DARKGRAY); y += 22;
    DrawText("1 / 2 / 3: animation speed", x + 18, y, 14, DARKGRAY); y += 22;
    DrawText("F1 / F2 / F3: camera views", x + 18, y, 14, DARKGRAY); y += 22;
    DrawText("Arrow keys + wheel: move / zoom", x + 18, y, 14, DARKGRAY); y += 22;
    DrawText("Home: overview   R: reset", x + 18, y, 14, DARKGRAY); y += 22;
    DrawText("Esc: return to console", x + 18, y, 14, DARKGRAY);
}

static int handle_camera_button_click(CameraView *view) {
    int i;
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return 0;
    for (i = 0; i < 3; ++i) {
        if (CheckCollisionPointRec(GetMousePosition(), camera_button(i))) {
            *view = (CameraView)i;
            return 1;
        }
    }
    return 0;
}

static void update_camera_controls(Camera3D *camera, const SceneLayout *layout) {
    float move = camera->fovy * GetFrameTime() * 0.55f;
    float wheel = GetMouseWheelMove();
    Vector3 shift = {0};

    if (IsKeyDown(KEY_UP)) shift.z -= move;
    if (IsKeyDown(KEY_DOWN)) shift.z += move;
    if (IsKeyDown(KEY_LEFT)) shift.x -= move;
    if (IsKeyDown(KEY_RIGHT)) shift.x += move;
    camera->position = Vector3Add(camera->position, shift);
    camera->target = Vector3Add(camera->target, shift);
    if (wheel != 0.0f) {
        camera->fovy *= powf(0.90f, wheel);
        camera->fovy = clamp_float(camera->fovy, 5.5f, layout->overview_size * 1.45f);
    }
}

void renderer3d_run(const Graph *graph) {
    SceneLayout layout;
    Camera3D camera;
    RendererState state = {0};
    int screenshot_mode = getenv("MSP_3D_SCREENSHOT") != NULL;
    int frame_count = 0;

    if (graph == NULL || graph->node_count <= 0) return;
    layout = build_scene_layout(graph);
    state.start_id = -1;
    state.goal_id = -1;
    state.hover_id = -1;
    state.algorithm = 1;
    state.animation_step = 0.22f;
    state.camera_view = VIEW_OVERVIEW;
    snprintf(state.message, sizeof(state.message), "Select two landmarks");
    camera = camera_for_view(graph, &layout, &state, state.camera_view);

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "SCU Jiang'an Campus - 3D Shortest Path");
    SetTargetFPS(60);
    if (screenshot_mode) {
        state.start_id = 0;
        state.goal_id = 13;
        run_search(graph, &state);
        state.animation_count = state.result.visited_count;
        state.camera_view = VIEW_NAVIGATION;
        camera = camera_for_view(graph, &layout, &state, state.camera_view);
    }

    while (!WindowShouldClose()) {
        int view_width = GetScreenWidth() - PANEL_WIDTH;
        int picked;
        int camera_changed = 0;

        if (IsKeyPressed(KEY_HOME) || IsKeyPressed(KEY_F1)) {
            state.camera_view = VIEW_OVERVIEW;
            camera_changed = 1;
        }
        if (IsKeyPressed(KEY_F2)) {
            state.camera_view = VIEW_NAVIGATION;
            camera_changed = 1;
        }
        if (IsKeyPressed(KEY_F3)) {
            state.camera_view = VIEW_PATH;
            camera_changed = 1;
        }
        if (handle_camera_button_click(&state.camera_view)) camera_changed = 1;
        if (camera_changed) {
            camera = camera_for_view(graph, &layout, &state, state.camera_view);
        }

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
            state.hover_id = -1;
            state.has_result = 0;
            state.animation_count = 0;
            snprintf(state.message, sizeof(state.message), "Selection reset");
        }

        state.hover_id = -1;
        if (GetMouseX() >= 0 && GetMouseX() < view_width &&
            GetMouseY() >= 0 && GetMouseY() < GetScreenHeight()) {
            state.hover_id = pick_node(graph, &layout,
                                       GetMouseRay(GetMousePosition(), camera));
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
                IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                picked = state.hover_id;
                if (picked >= 0) {
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) state.start_id = picked;
                    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) state.goal_id = picked;
                    state.has_result = 0;
                    snprintf(state.message, sizeof(state.message), "Press Enter to search");
                }
            }
        }

        update_camera_controls(&camera, &layout);
        update_animation(&state);

        BeginDrawing();
        ClearBackground((Color){222, 231, 224, 255});
        BeginMode3D(camera);
        DrawPlane((Vector3){0.0f, -0.015f, 0.0f},
                  (Vector2){layout.max_x - layout.min_x + 4.0f,
                            layout.max_z - layout.min_z + 4.0f},
                  (Color){205, 218, 205, 255});
        draw_roads(graph, &layout, &state);
        {
            int i;
            for (i = 0; i < graph->node_count; ++i) {
                draw_node(&graph->nodes[i], &layout, &state);
            }
        }
        EndMode3D();
        draw_labels(graph, &layout, &camera, &state, view_width);
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
