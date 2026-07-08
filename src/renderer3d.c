#include "renderer3d.h"

#include "astar.h"
#include "dijkstra.h"
#include "graph.h"

#include "raylib.h"
#include "raymath.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define NAV_WIDTH 300
#define MAP_WIDTH (WINDOW_WIDTH - NAV_WIDTH)
#define MAP_HEIGHT WINDOW_HEIGHT
#define RENDER_SCALE_X 2.20f
#define RENDER_SCALE_Z 1.78f
#define BUILDING_SCALE 0.76f
#define HEIGHT_SCALE 0.42f
#define ROAD_HEIGHT 0.035f
#define PATH_HEIGHT 0.070f

typedef enum { VIEW_OVERVIEW = 0, VIEW_NAVIGATION, VIEW_PATH } CameraView;
typedef enum { AREA_GRASS = 0, AREA_LAKE, AREA_SPORTS, AREA_SQUARE } AreaType;

typedef struct {
    const char *name;
    AreaType type;
    float x;
    float y;
    float width;
    float depth;
} MapArea;

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
    int search_match_id;
    int search_focus;
    int algorithm;
    int paused;
    int animation_count;
    float animation_timer;
    float animation_step;
    double search_ms;
    PathResult result;
    int has_result;
    int display_3d;
    CameraView camera_view;
    char search_query[64];
    char message[128];
} RendererState;

static const MapArea MAP_AREAS[] = {
    {"West green", AREA_GRASS, 3.5f, 9.6f, 4.2f, 4.0f},
    {"Lake green", AREA_GRASS, 7.0f, 6.0f, 4.8f, 3.6f},
    {"Teaching green", AREA_GRASS, 10.8f, 6.2f, 4.6f, 4.0f},
    {"East green", AREA_GRASS, 14.0f, 4.2f, 4.2f, 5.0f},
    {"Mingyuan Lake", AREA_LAKE, 7.0f, 6.0f, 1.9f, 1.5f},
    {"West Sports", AREA_SPORTS, 5.0f, 11.0f, 2.2f, 1.45f},
    {"Youth Square", AREA_SQUARE, 5.0f, 8.0f, 1.55f, 1.25f},
    {"Knowledge Square", AREA_SQUARE, 9.0f, 7.0f, 1.55f, 1.15f}
};

static const Vector2 TREE_POINTS[] = {
    {3.2f, 10.8f}, {4.1f, 9.2f}, {5.9f, 9.5f}, {6.0f, 7.4f},
    {5.6f, 5.9f}, {6.2f, 5.2f}, {7.9f, 6.9f}, {8.3f, 4.5f},
    {9.8f, 8.4f}, {10.3f, 6.2f}, {10.2f, 4.2f}, {11.8f, 6.3f},
    {12.7f, 7.9f}, {13.8f, 8.2f}, {13.9f, 5.0f}, {14.6f, 4.2f},
    {14.8f, 2.2f}, {12.1f, 2.8f}, {9.4f, 2.0f}, {6.8f, 3.8f}
};

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
        fmaxf(layout.max_z - layout.min_z, (layout.max_x - layout.min_x) / 1.34f) * 1.22f;
    return layout;
}

static Vector3 world_position(float x, float y, const SceneLayout *layout, float height) {
    return (Vector3){(x - layout->center_x) * RENDER_SCALE_X,
                     height,
                     (y - layout->center_z) * RENDER_SCALE_Z};
}

static Vector3 node_position(const Node *node, const SceneLayout *layout, float height) {
    return world_position((float)node->x, (float)node->y, layout, height);
}

static float node_render_height(const Node *node) {
    if (node->type == NODE_LAKE || node->type == NODE_SQUARE) return 0.018f;
    if (node->type == NODE_JUNCTION) return 0.018f;
    if (node->type == NODE_GATE) return clamp_float(node->height * HEIGHT_SCALE, 0.10f, 0.18f);
    return clamp_float(node->height * HEIGHT_SCALE, 0.12f, 0.29f);
}

static float node_render_width(const Node *node) {
    return fmaxf(node->width * RENDER_SCALE_X * BUILDING_SCALE, 0.24f);
}

static float node_render_depth(const Node *node) {
    return fmaxf(node->depth * RENDER_SCALE_Z * BUILDING_SCALE, 0.24f);
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
            (result->path[i] == to_id && result->path[i + 1] == from_id)) return 1;
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
        state->animation_count > state->result.visited_count) return -1;
    return state->result.visited_order[state->animation_count - 1];
}

static Camera3D camera_for_view(const Graph *graph, const SceneLayout *layout,
                                const RendererState *state, CameraView view) {
    Camera3D camera = {0};
    Vector3 target = {0.0f, 0.0f, 0.0f};
    float size = layout->overview_size;
    if (view == VIEW_PATH && state->has_result && state->result.found) {
        float min_x = 1.0e30f, max_x = -1.0e30f;
        float min_z = 1.0e30f, max_z = -1.0e30f;
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
            size = fmaxf(max_z - min_z, (max_x - min_x) / 1.34f) * 1.38f;
            size = clamp_float(size, 8.0f, layout->overview_size);
        }
    }
    camera.target = target;
    if (state->display_3d) {
        camera.position = (Vector3){target.x + size * 0.46f, size * 0.82f,
                                    target.z + size * 0.62f};
        camera.fovy = 38.0f;
        camera.projection = CAMERA_PERSPECTIVE;
    } else {
        float tilt = view == VIEW_NAVIGATION ? 0.22f : 0.34f;
        camera.position = (Vector3){target.x + size * 0.10f, size * 1.18f,
                                    target.z + size * tilt};
        camera.fovy = view == VIEW_NAVIGATION ? size * 0.94f : size;
        camera.projection = CAMERA_ORTHOGRAPHIC;
    }
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    return camera;
}

static Color area_color(AreaType type) {
    switch (type) {
        case AREA_LAKE: return (Color){112, 184, 218, 255};
        case AREA_SPORTS: return (Color){112, 166, 112, 255};
        case AREA_SQUARE: return (Color){221, 211, 180, 255};
        case AREA_GRASS:
        default: return (Color){190, 217, 179, 255};
    }
}

static void draw_environment(const SceneLayout *layout) {
    int i;
    float ground_width = layout->max_x - layout->min_x + 5.5f;
    float ground_depth = layout->max_z - layout->min_z + 5.0f;
    DrawPlane((Vector3){0.0f, -0.035f, 0.0f}, (Vector2){ground_width, ground_depth},
              (Color){224, 230, 214, 255});
    DrawCubeWires((Vector3){0.0f, -0.025f, 0.0f}, ground_width, 0.02f, ground_depth,
                  Fade((Color){118, 143, 121, 255}, 0.34f));
    for (i = 0; i < (int)(sizeof(MAP_AREAS) / sizeof(MAP_AREAS[0])); ++i) {
        const MapArea *area = &MAP_AREAS[i];
        Vector3 center = world_position(area->x, area->y, layout, -0.012f);
        float width = area->width * RENDER_SCALE_X;
        float depth = area->depth * RENDER_SCALE_Z;
        DrawCube(center, width, 0.025f, depth, area_color(area->type));
        if (area->type != AREA_GRASS) {
            DrawCubeWires(center, width, 0.027f, depth,
                          Fade((Color){77, 104, 91, 255}, 0.45f));
        }
        if (area->type == AREA_SPORTS) {
            DrawCubeWires(center, width * 0.82f, 0.029f, depth * 0.70f,
                          (Color){190, 115, 91, 210});
        }
    }
}

static void draw_tree(float x, float y, const SceneLayout *layout) {
    Vector3 base = world_position(x, y, layout, 0.0f);
    DrawCylinder((Vector3){base.x, 0.10f, base.z}, 0.035f, 0.045f, 0.20f, 8,
                 (Color){116, 88, 57, 255});
    DrawSphere((Vector3){base.x, 0.27f, base.z}, 0.16f,
               (Color){91, 145, 87, 255});
}

static void draw_trees(const SceneLayout *layout) {
    int i;
    for (i = 0; i < (int)(sizeof(TREE_POINTS) / sizeof(TREE_POINTS[0])); ++i) {
        draw_tree(TREE_POINTS[i].x, TREE_POINTS[i].y, layout);
    }
}

static int name_contains(const char *text, const char *needle) {
    size_t i, j;
    if (text == NULL || needle == NULL || needle[0] == '\0') return 0;
    for (i = 0; text[i] != '\0'; ++i) {
        for (j = 0; needle[j] != '\0' && text[i + j] != '\0'; ++j) {
            if (tolower((unsigned char)text[i + j]) !=
                tolower((unsigned char)needle[j])) break;
        }
        if (needle[j] == '\0') return 1;
    }
    return 0;
}

static Color building_color(const Node *node) {
    if (name_contains(node->name, "Canteen")) return (Color){224, 164, 98, 255};
    if (name_contains(node->name, "Dorm")) return (Color){137, 177, 145, 255};
    if (name_contains(node->name, "Hospital")) return (Color){210, 132, 127, 255};
    if (name_contains(node->name, "Library")) return (Color){102, 145, 173, 255};
    if (name_contains(node->name, "Gym")) return (Color){148, 126, 170, 255};
    if (name_contains(node->name, "College") || name_contains(node->name, "Teaching") ||
        name_contains(node->name, "Basic")) return (Color){124, 157, 181, 255};
    return (Color){151, 164, 158, 255};
}

static void draw_gate(const Node *node, const SceneLayout *layout, Color color) {
    Vector3 center = node_position(node, layout, 0.0f);
    float width = fmaxf(node_render_width(node), 0.95f);
    float height = node_render_height(node);
    float depth = fmaxf(node_render_depth(node), 0.38f);
    Vector3 left = {center.x - width * 0.40f, height * 0.5f, center.z};
    Vector3 right = {center.x + width * 0.40f, height * 0.5f, center.z};
    DrawCube(left, width * 0.14f, height, depth, color);
    DrawCube(right, width * 0.14f, height, depth, color);
    DrawCube((Vector3){center.x, height, center.z}, width, height * 0.22f, depth, color);
}

static void draw_node(const Node *node, const SceneLayout *layout,
                      const RendererState *state) {
    Vector3 center = node_position(node, layout, 0.0f);
    float width = node_render_width(node);
    float depth = node_render_depth(node);
    float height = node_render_height(node);
    Color color;
    if (node->type == NODE_JUNCTION) return;
    if (node->type == NODE_LAKE || node->type == NODE_SQUARE || node->id == 4) return;
    if (node->id == 16) {
        DrawCube((Vector3){center.x, 0.012f, center.z}, width, 0.024f, depth,
                 (Color){98, 174, 211, 255});
        DrawCubeWires((Vector3){center.x, 0.014f, center.z}, width, 0.026f, depth,
                      (Color){55, 119, 159, 190});
        return;
    }
    color = building_color(node);
    if (node->id == state->start_id) color = (Color){53, 174, 104, 255};
    if (node->id == state->goal_id) color = (Color){224, 86, 78, 255};
    if (node->type == NODE_GATE) {
        draw_gate(node, layout, color);
        return;
    }
    DrawCube((Vector3){center.x + 0.08f, 0.025f, center.z + 0.08f},
             width, 0.025f, depth, Fade((Color){72, 83, 80, 255}, 0.22f));
    center.y = height * 0.5f;
    DrawCube(center, width, height, depth, color);
    DrawCubeWires(center, width, height, depth, Fade((Color){58, 72, 72, 255}, 0.52f));
    DrawCube((Vector3){center.x, height + 0.012f, center.z},
             width * 0.88f, 0.025f, depth * 0.88f, Fade(RAYWHITE, 0.24f));
}

static void draw_band(Vector3 from, Vector3 to, float width, Color color) {
    Vector3 points[4];
    float dx = to.x - from.x;
    float dz = to.z - from.z;
    float length = sqrtf(dx * dx + dz * dz);
    float ox, oz;
    if (length <= 0.0001f) return;
    ox = -dz / length * width * 0.5f;
    oz = dx / length * width * 0.5f;
    points[0] = (Vector3){from.x - ox, from.y, from.z - oz};
    points[1] = (Vector3){from.x + ox, from.y, from.z + oz};
    points[2] = (Vector3){to.x - ox, to.y, to.z - oz};
    points[3] = (Vector3){to.x + ox, to.y, to.z + oz};
    DrawTriangleStrip3D(points, 4, color);
}

static float road_width(const Node *from, const Node *to) {
    if (from->type == NODE_JUNCTION && to->type == NODE_JUNCTION) return 0.38f;
    if ((from->type == NODE_GATE && to->type == NODE_JUNCTION) ||
        (to->type == NODE_GATE && from->type == NODE_JUNCTION)) return 0.32f;
    if (from->type == NODE_JUNCTION || to->type == NODE_JUNCTION) return 0.25f;
    return 0.16f;
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
        float width;
        if (from == NULL || to == NULL) continue;
        width = road_width(from, to);
        draw_band(node_position(from, layout, ROAD_HEIGHT - 0.004f),
                  node_position(to, layout, ROAD_HEIGHT - 0.004f), width + 0.10f,
                  (Color){245, 245, 237, 255});
        draw_band(node_position(from, layout, ROAD_HEIGHT),
                  node_position(to, layout, ROAD_HEIGHT), width,
                  (Color){194, 199, 194, 255});
    }
    if (!show_path) return;
    for (i = 0; i < graph->edge_count; ++i) {
        const Edge *edge = &graph->edges[i];
        const Node *from, *to;
        if (!result_contains_edge(&state->result, edge->from_id, edge->to_id)) continue;
        from = graph_get_node(graph, edge->from_id);
        to = graph_get_node(graph, edge->to_id);
        if (from == NULL || to == NULL) continue;
        draw_band(node_position(from, layout, PATH_HEIGHT),
                  node_position(to, layout, PATH_HEIGHT), 0.46f,
                  (Color){242, 143, 55, 255});
    }
}

static void draw_search_markers(const Graph *graph, const SceneLayout *layout,
                                const RendererState *state) {
    int i;
    for (i = 0; i < graph->node_count; ++i) {
        const Node *node = &graph->nodes[i];
        Vector3 position;
        if (!visited_contains(state, node->id) && node->id != current_node_id(state)) continue;
        position = node_position(node, layout, 0.095f);
        DrawSphere(position, node->id == current_node_id(state) ? 0.14f : 0.075f,
                   node->id == current_node_id(state) ? (Color){38, 119, 184, 255}
                                                      : (Color){95, 157, 201, 180});
    }
}

static int pick_node(const Graph *graph, const SceneLayout *layout, Ray ray) {
    int i, picked = -1;
    float nearest = 1.0e30f;
    for (i = 0; i < graph->node_count; ++i) {
        const Node *node = &graph->nodes[i];
        Vector3 center, half;
        BoundingBox box;
        RayCollision hit;
        float height;
        if (node->type == NODE_JUNCTION) continue;
        height = fmaxf(node_render_height(node), 0.08f);
        center = node_position(node, layout, height * 0.5f);
        half = (Vector3){fmaxf(node_render_width(node) * 0.55f, 0.32f),
                         fmaxf(height, 0.18f),
                         fmaxf(node_render_depth(node) * 0.55f, 0.32f)};
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
        snprintf(state->message, sizeof(state->message), "Choose start and destination");
        return;
    }
    if (state->start_id == state->goal_id) {
        snprintf(state->message, sizeof(state->message), "Start and destination must differ");
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
    snprintf(state->message, sizeof(state->message),
             status == MSP_OK ? "Planning the recommended route..." : "No route found");
}

static void update_animation(RendererState *state) {
    if (!state->has_result || state->paused ||
        state->animation_count >= state->result.visited_count) return;
    state->animation_timer += GetFrameTime();
    if (state->animation_timer >= state->animation_step) {
        state->animation_timer = 0.0f;
        state->animation_count++;
        if (state->animation_count >= state->result.visited_count) {
            snprintf(state->message, sizeof(state->message), "Recommended route ready");
        }
    }
}

static int is_important_landmark(int id) {
    return id == 0 || id == 4 || id == 7 || id == 12 || id == 13 ||
           id == 14 || id == 15;
}

static int label_visible(const Node *node, const Camera3D *camera,
                         const SceneLayout *layout, const RendererState *state) {
    if (node->type == NODE_JUNCTION) return 0;
    if (node->id == state->start_id || node->id == state->goal_id ||
        node->id == state->hover_id || node->id == state->search_match_id) return 1;
    if (is_important_landmark(node->id)) return 1;
    return camera->projection == CAMERA_ORTHOGRAPHIC &&
           camera->fovy < layout->overview_size * 0.58f;
}

static void draw_labels(const Graph *graph, const SceneLayout *layout,
                        const Camera3D *camera, const RendererState *state) {
    Rectangle occupied[MSP_MAX_NODES];
    int occupied_count = 0;
    int pass, i;
    for (pass = 0; pass < 2; ++pass) {
        for (i = 0; i < graph->node_count; ++i) {
            const Node *node = &graph->nodes[i];
            int priority = node->id == state->start_id || node->id == state->goal_id ||
                           node->id == state->hover_id || node->id == state->search_match_id;
            Vector2 screen;
            Rectangle bounds;
            int j, collision = 0;
            int font_size = priority ? 15 : 13;
            if (priority != (pass == 0) || !label_visible(node, camera, layout, state)) continue;
            screen = GetWorldToScreenEx(
                node_position(node, layout, node_render_height(node) + 0.17f),
                *camera, MAP_WIDTH, MAP_HEIGHT);
            if (screen.x < 5 || screen.x > MAP_WIDTH - 5 ||
                screen.y < 5 || screen.y > MAP_HEIGHT - 5) continue;
            bounds = (Rectangle){screen.x - MeasureText(node->name, font_size) * 0.5f - 5.0f,
                                 screen.y - font_size - 6.0f,
                                 (float)MeasureText(node->name, font_size) + 10.0f,
                                 (float)font_size + 7.0f};
            for (j = 0; j < occupied_count; ++j) {
                if (CheckCollisionRecs(bounds, occupied[j])) { collision = 1; break; }
            }
            if (collision && !priority) continue;
            DrawRectangleRounded(bounds, 0.18f, 4,
                                 priority ? (Color){255, 255, 250, 238}
                                          : (Color){248, 249, 242, 220});
            DrawText(node->name, (int)bounds.x + 5, (int)bounds.y + 3,
                     font_size, (Color){44, 56, 55, 255});
            if (occupied_count < MSP_MAX_NODES) occupied[occupied_count++] = bounds;
        }
    }
}

static Rectangle search_box(void) {
    return (Rectangle){NAV_WIDTH + 24.0f, 18.0f, 390.0f, 42.0f};
}

static Rectangle algorithm_button(int algorithm) {
    return (Rectangle){20.0f + (algorithm - 1) * 127.0f, 254.0f, 123.0f, 34.0f};
}

static Rectangle navigate_button(void) {
    return (Rectangle){20.0f, 310.0f, 260.0f, 42.0f};
}

static Rectangle tool_button(int index) {
    return (Rectangle){WINDOW_WIDTH - 58.0f, WINDOW_HEIGHT - 206.0f + index * 43.0f,
                       38.0f, 36.0f};
}

static void draw_text_fit(const char *text, int x, int y, int max_width,
                          int font_size, Color color) {
    char buffer[64];
    size_t length = strlen(text);
    if (MeasureText(text, font_size) <= max_width) {
        DrawText(text, x, y, font_size, color);
        return;
    }
    if (length > 54) length = 54;
    memcpy(buffer, text, length);
    buffer[length] = '\0';
    while (length > 3 && MeasureText(buffer, font_size) > max_width - 15) {
        buffer[--length] = '\0';
    }
    strcat(buffer, "...");
    DrawText(buffer, x, y, font_size, color);
}

static void draw_navigation_panel(const Graph *graph, const RendererState *state) {
    const Node *start = graph_get_node(graph, state->start_id);
    const Node *goal = graph_get_node(graph, state->goal_id);
    int y;
    DrawRectangle(0, 0, NAV_WIDTH, WINDOW_HEIGHT, (Color){249, 250, 246, 255});
    DrawLine(NAV_WIDTH - 1, 0, NAV_WIDTH - 1, WINDOW_HEIGHT,
             (Color){196, 205, 198, 255});
    DrawText("SCU JIANG'AN", 20, 20, 13, (Color){76, 116, 100, 255});
    DrawText("Campus Navigation", 20, 42, 24, (Color){33, 66, 57, 255});
    DrawText("START", 20, 91, 11, (Color){98, 111, 105, 255});
    DrawRectangleRounded((Rectangle){20, 108, 260, 48}, 0.12f, 5,
                         (Color){235, 244, 236, 255});
    DrawCircle(38, 132, 6, (Color){48, 170, 99, 255});
    draw_text_fit(start ? start->name : "Click a place on the map", 53, 123, 215, 15,
                  (Color){44, 63, 57, 255});
    DrawText("DESTINATION", 20, 169, 11, (Color){98, 111, 105, 255});
    DrawRectangleRounded((Rectangle){20, 186, 260, 48}, 0.12f, 5,
                         (Color){249, 236, 232, 255});
    DrawCircle(38, 210, 6, (Color){221, 78, 69, 255});
    draw_text_fit(goal ? goal->name : "Right-click a destination", 53, 201, 215, 15,
                  (Color){44, 63, 57, 255});
    DrawText("ROUTE MODE", 20, 241, 11, (Color){98, 111, 105, 255});
    {
        int algorithm;
        for (algorithm = 1; algorithm <= 2; ++algorithm) {
            Rectangle button = algorithm_button(algorithm);
            int active = state->algorithm == algorithm;
            DrawRectangleRounded(button, 0.14f, 5,
                                 active ? (Color){55, 112, 93, 255}
                                        : (Color){229, 234, 228, 255});
            DrawText(algorithm == 1 ? "Dijkstra" : "A*",
                     (int)button.x + (algorithm == 1 ? 31 : 52), (int)button.y + 9,
                     14, active ? RAYWHITE : (Color){69, 82, 77, 255});
        }
    }
    DrawRectangleRounded(navigate_button(), 0.12f, 5, (Color){230, 139, 48, 255});
    DrawText("Start navigation", 87, 322, 16, RAYWHITE);
    DrawText(state->message, 20, 367, 13, (Color){77, 99, 90, 255});
    DrawLine(20, 396, 280, 396, (Color){218, 224, 217, 255});
    DrawText("ROUTE DETAILS", 20, 414, 11, (Color){98, 111, 105, 255});
    if (!state->has_result) {
        DrawText("Select two places to see", 20, 442, 15, (Color){119, 128, 124, 255});
        DrawText("distance and walking time.", 20, 463, 15, (Color){119, 128, 124, 255});
        return;
    }
    DrawText(TextFormat("%.1f", state->result.total_distance), 20, 440, 30,
             (Color){42, 72, 62, 255});
    DrawText("distance units", 84, 451, 13, (Color){111, 123, 117, 255});
    DrawText(TextFormat("About %.0f min walk", fmax(1.0, state->result.total_distance * 1.25)),
             20, 480, 15, (Color){70, 84, 79, 255});
    DrawText(TextFormat("%d route points", state->result.path_length),
             165, 480, 15, (Color){70, 84, 79, 255});
    DrawText("ROUTE", 20, 515, 11, (Color){98, 111, 105, 255});
    y = 538;
    {
        int i;
        int shown = state->result.path_length < 6 ? state->result.path_length : 6;
        for (i = 0; i < shown; ++i) {
            const Node *node = graph_get_node(graph, state->result.path[i]);
            if (node == NULL) continue;
            DrawCircle(27, y + 7, node->type == NODE_JUNCTION ? 3 : 5,
                       node->type == NODE_JUNCTION ? (Color){153, 161, 157, 255}
                                                   : (Color){230, 139, 48, 255});
            draw_text_fit(node->name, 40, y, 230, 13, (Color){63, 76, 71, 255});
            y += 25;
        }
        if (state->result.path_length > shown) {
            DrawText(TextFormat("+ %d more points", state->result.path_length - shown),
                     40, y, 12, (Color){112, 122, 118, 255});
        }
    }
}

static void draw_search_ui(const Graph *graph, const RendererState *state) {
    Rectangle box = search_box();
    const Node *match = graph_get_node(graph, state->search_match_id);
    DrawRectangleRounded(box, 0.15f, 6, (Color){255, 255, 252, 245});
    DrawRectangleRoundedLinesEx(box, 0.15f, 6, 1.0f,
                                state->search_focus ? (Color){67, 128, 107, 255}
                                                    : (Color){186, 198, 190, 255});
    DrawCircleLines((int)box.x + 19, (int)box.y + 19, 7, (Color){83, 101, 94, 255});
    DrawLine((int)box.x + 24, (int)box.y + 25, (int)box.x + 29, (int)box.y + 30,
             (Color){83, 101, 94, 255});
    DrawText(state->search_query[0] ? state->search_query : "Search library, canteen, gym...",
             (int)box.x + 39, (int)box.y + 12, 16,
             state->search_query[0] ? (Color){50, 65, 59, 255}
                                    : (Color){132, 143, 138, 255});
    if (state->search_focus && match != NULL) {
        Rectangle result = {box.x, box.y + box.height + 5, box.width, 38};
        DrawRectangleRounded(result, 0.12f, 5, (Color){255, 255, 252, 248});
        DrawText(TextFormat("[%02d] %s", match->id, match->name),
                 (int)result.x + 12, (int)result.y + 10, 14, (Color){49, 72, 62, 255});
    }
}

static void draw_legend(void) {
    int x = WINDOW_WIDTH - 184;
    int y = 18;
    const char *labels[] = {"Teaching", "Dorm / dining", "Green space", "Water", "Road", "Route"};
    Color colors[] = {{124,157,181,255}, {224,164,98,255}, {190,217,179,255},
                      {112,184,218,255}, {194,199,194,255}, {242,143,55,255}};
    int i;
    DrawRectangleRounded((Rectangle){(float)x, (float)y, 164, 190}, 0.06f, 6,
                         (Color){255, 255, 252, 238});
    DrawText("MAP LEGEND", x + 14, y + 14, 12, (Color){73, 91, 83, 255});
    for (i = 0; i < 6; ++i) {
        DrawRectangle(x + 15, y + 43 + i * 23, 16, 9, colors[i]);
        DrawText(labels[i], x + 41, y + 39 + i * 23, 13, (Color){58, 70, 66, 255});
    }
}

static void draw_tool_button(int index, const char *label, int active) {
    Rectangle button = tool_button(index);
    DrawRectangleRounded(button, 0.16f, 5,
                         active ? (Color){57, 111, 94, 255}
                                : (Color){255, 255, 252, 240});
    DrawRectangleRoundedLinesEx(button, 0.16f, 5, 1.0f, (Color){174, 187, 178, 255});
    DrawText(label, (int)(button.x + button.width * 0.5f - MeasureText(label, 16) * 0.5f),
             (int)button.y + 10, 16, active ? RAYWHITE : (Color){55, 72, 64, 255});
}

static void draw_map_tools(const RendererState *state) {
    draw_tool_button(0, "+", 0);
    draw_tool_button(1, "-", 0);
    draw_tool_button(2, "H", 0);
    draw_tool_button(3, state->display_3d ? "3D" : "2.5D", 1);
}

static int update_search(const Graph *graph, RendererState *state) {
    int key;
    size_t length;
    int i;
    int changed = 0;
    if (!state->search_focus) return 0;
    key = GetCharPressed();
    while (key > 0) {
        length = strlen(state->search_query);
        if (key >= 32 && key <= 126 && length + 1 < sizeof(state->search_query)) {
            state->search_query[length] = (char)key;
            state->search_query[length + 1] = '\0';
            changed = 1;
        }
        key = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE) && state->search_query[0]) {
        state->search_query[strlen(state->search_query) - 1] = '\0';
        changed = 1;
    }
    if (changed || state->search_match_id < 0) {
        state->search_match_id = -1;
        if (state->search_query[0]) {
            for (i = 0; i < graph->node_count; ++i) {
                if (graph->nodes[i].type != NODE_JUNCTION &&
                    name_contains(graph->nodes[i].name, state->search_query)) {
                    state->search_match_id = graph->nodes[i].id;
                    break;
                }
            }
        }
    }
    return changed;
}

static void update_camera_controls(Camera3D *camera, const SceneLayout *layout,
                                   int over_map) {
    float move = (camera->projection == CAMERA_ORTHOGRAPHIC ? camera->fovy : 18.0f) *
                 GetFrameTime() * 0.52f;
    float wheel = over_map ? GetMouseWheelMove() : 0.0f;
    Vector3 shift = {0};
    if (IsKeyDown(KEY_UP)) shift.z -= move;
    if (IsKeyDown(KEY_DOWN)) shift.z += move;
    if (IsKeyDown(KEY_LEFT)) shift.x -= move;
    if (IsKeyDown(KEY_RIGHT)) shift.x += move;
    camera->position = Vector3Add(camera->position, shift);
    camera->target = Vector3Add(camera->target, shift);
    if (wheel != 0.0f) {
        if (camera->projection == CAMERA_ORTHOGRAPHIC) {
            camera->fovy = clamp_float(camera->fovy * powf(0.90f, wheel),
                                       6.0f, layout->overview_size * 1.45f);
        } else {
            Vector3 offset = Vector3Subtract(camera->position, camera->target);
            float factor = powf(0.90f, wheel);
            camera->position = Vector3Add(camera->target, Vector3Scale(offset, factor));
        }
    }
}

void renderer3d_run(const Graph *graph) {
    SceneLayout layout;
    Camera3D camera;
    RendererState state = {0};
    RenderTexture2D map_target;
    int screenshot_mode = getenv("MSP_3D_SCREENSHOT") != NULL;
    int frame_count = 0;
    if (graph == NULL || graph->node_count <= 0) return;
    layout = build_scene_layout(graph);
    state.start_id = -1;
    state.goal_id = -1;
    state.hover_id = -1;
    state.search_match_id = -1;
    state.algorithm = 1;
    state.animation_step = 0.18f;
    state.camera_view = VIEW_NAVIGATION;
    snprintf(state.message, sizeof(state.message), "Choose your route");
    camera = camera_for_view(graph, &layout, &state, state.camera_view);

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "SCU Jiang'an Campus Navigation");
    map_target = LoadRenderTexture(MAP_WIDTH, MAP_HEIGHT);
    SetTargetFPS(60);
    if (screenshot_mode) {
        state.start_id = 0;
        state.goal_id = 13;
        run_search(graph, &state);
        state.animation_count = state.result.visited_count;
        snprintf(state.message, sizeof(state.message), "Recommended route ready");
        state.camera_view = VIEW_PATH;
        camera = camera_for_view(graph, &layout, &state, state.camera_view);
    }

    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();
        int over_map = mouse.x >= NAV_WIDTH && mouse.x < WINDOW_WIDTH;
        int map_ui_hit = CheckCollisionPointRec(mouse, search_box()) ||
                         CheckCollisionPointRec(mouse, (Rectangle){WINDOW_WIDTH - 184, 18, 164, 190});
        int camera_changed = 0;
        int picked;
        int i;

        for (i = 0; i < 4; ++i) {
            if (CheckCollisionPointRec(mouse, tool_button(i))) map_ui_hit = 1;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointRec(mouse, search_box())) {
                state.search_focus = 1;
            } else {
                state.search_focus = 0;
            }
            for (i = 1; i <= 2; ++i) {
                if (CheckCollisionPointRec(mouse, algorithm_button(i))) state.algorithm = i;
            }
            if (CheckCollisionPointRec(mouse, navigate_button())) {
                run_search(graph, &state);
                if (state.has_result) {
                    state.camera_view = VIEW_PATH;
                    camera_changed = 1;
                }
            }
            for (i = 0; i < 4; ++i) {
                if (!CheckCollisionPointRec(mouse, tool_button(i))) continue;
                if (i == 0 && camera.projection == CAMERA_ORTHOGRAPHIC) camera.fovy *= 0.86f;
                if (i == 1 && camera.projection == CAMERA_ORTHOGRAPHIC) camera.fovy *= 1.16f;
                if (i == 2) { state.camera_view = VIEW_NAVIGATION; camera_changed = 1; }
                if (i == 3) { state.display_3d = !state.display_3d; camera_changed = 1; }
            }
        }

        update_search(graph, &state);
        if (state.search_focus && IsKeyPressed(KEY_ENTER) && state.search_match_id >= 0) {
            if (state.start_id < 0) state.start_id = state.search_match_id;
            else state.goal_id = state.search_match_id;
            state.search_focus = 0;
            state.has_result = 0;
            snprintf(state.message, sizeof(state.message), "Press Start navigation");
        } else if (!state.search_focus && IsKeyPressed(KEY_ENTER)) {
            run_search(graph, &state);
            if (state.has_result) { state.camera_view = VIEW_PATH; camera_changed = 1; }
        }

        if (IsKeyPressed(KEY_D)) state.algorithm = 1;
        if (IsKeyPressed(KEY_A)) state.algorithm = 2;
        if (IsKeyPressed(KEY_SPACE)) state.paused = !state.paused;
        if (IsKeyPressed(KEY_HOME) || IsKeyPressed(KEY_F1)) {
            state.camera_view = VIEW_NAVIGATION; camera_changed = 1;
        }
        if (IsKeyPressed(KEY_F2)) { state.camera_view = VIEW_OVERVIEW; camera_changed = 1; }
        if (IsKeyPressed(KEY_F3)) { state.camera_view = VIEW_PATH; camera_changed = 1; }
        if (IsKeyPressed(KEY_R)) {
            state.start_id = state.goal_id = -1;
            state.has_result = 0;
            snprintf(state.message, sizeof(state.message), "Route cleared");
        }
        if (camera_changed) camera = camera_for_view(graph, &layout, &state, state.camera_view);

        state.hover_id = -1;
        if (over_map && !map_ui_hit) {
            Vector2 local_mouse = {mouse.x - NAV_WIDTH, mouse.y};
            state.hover_id = pick_node(graph, &layout,
                GetScreenToWorldRayEx(local_mouse, camera, MAP_WIDTH, MAP_HEIGHT));
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
                IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                picked = state.hover_id;
                if (picked >= 0) {
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) state.start_id = picked;
                    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) state.goal_id = picked;
                    state.has_result = 0;
                    snprintf(state.message, sizeof(state.message), "Press Start navigation");
                }
            }
        }
        update_camera_controls(&camera, &layout, over_map && !map_ui_hit);
        update_animation(&state);

        BeginTextureMode(map_target);
        ClearBackground((Color){213, 226, 219, 255});
        BeginMode3D(camera);
        draw_environment(&layout);
        draw_roads(graph, &layout, &state);
        for (i = 0; i < graph->node_count; ++i) draw_node(&graph->nodes[i], &layout, &state);
        draw_trees(&layout);
        draw_search_markers(graph, &layout, &state);
        EndMode3D();
        draw_labels(graph, &layout, &camera, &state);
        EndTextureMode();

        BeginDrawing();
        ClearBackground((Color){231, 237, 232, 255});
        DrawTextureRec(map_target.texture,
                       (Rectangle){0, 0, (float)MAP_WIDTH, -(float)MAP_HEIGHT},
                       (Vector2){NAV_WIDTH, 0}, WHITE);
        draw_navigation_panel(graph, &state);
        draw_search_ui(graph, &state);
        draw_legend();
        draw_map_tools(&state);
        EndDrawing();

        frame_count++;
        if (screenshot_mode && frame_count == 12) {
            TakeScreenshot("3d-preview.png");
            break;
        }
    }
    UnloadRenderTexture(map_target);
    CloseWindow();
}
