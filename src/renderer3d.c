#include "renderer3d.h"

#include "astar.h"
#include "dijkstra.h"
#include "graph.h"
#include "storage.h"

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
#define TARGET_WORLD_WIDTH 30.0f
#define TARGET_WORLD_DEPTH 20.0f
#define BUILDING_SCALE 0.76f
#define HEIGHT_SCALE 0.42f
#define ROAD_HEIGHT 0.035f
#define PATH_HEIGHT 0.070f

static const Color COLOR_MAP_SKY = {225, 237, 231, 255};
static const Color COLOR_MAP_GROUND = {239, 240, 226, 255};
static const Color COLOR_GRASS = {195, 216, 184, 255};
static const Color COLOR_WATER = {112, 188, 224, 255};
static const Color COLOR_WATER_BORDER = {71, 143, 184, 210};
static const Color COLOR_ROAD_BORDER = {181, 188, 184, 255};
static const Color COLOR_ROAD_MAIN = {251, 250, 244, 255};
static const Color COLOR_ROAD_BRANCH = {244, 245, 239, 245};
static const Color COLOR_ROAD_ENTRANCE = {236, 239, 234, 185};
static const Color COLOR_ROAD_BRIDGE = {226, 219, 199, 255};
static const Color COLOR_ROUTE_SHADOW = {255, 255, 250, 245};
static const Color COLOR_ROUTE = {239, 126, 40, 255};
static const Color COLOR_START = {42, 166, 96, 255};
static const Color COLOR_GOAL = {218, 73, 66, 255};
static const Color COLOR_ACCENT = {48, 112, 91, 255};
static const Color COLOR_TEXT = {47, 61, 56, 255};
static const Color COLOR_PANEL = {251, 252, 249, 255};
static const Color COLOR_CARD = {255, 255, 252, 255};

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
    float scale_x;
    float scale_z;
    float min_x;
    float max_x;
    float min_z;
    float max_z;
    float overview_size;
    int legacy_decorations;
} SceneLayout;

typedef struct {
    int start_id;
    int goal_id;
    int start_place_id;
    int goal_place_id;
    int hover_id;
    int search_match_id;
    int search_match_place_id;
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
    layout.legacy_decorations = max_x - min_x <= 50.0 && max_y - min_y <= 50.0;
    if (layout.legacy_decorations) {
        layout.scale_x = RENDER_SCALE_X;
        layout.scale_z = RENDER_SCALE_Z;
    } else {
        layout.scale_x = max_x > min_x
                             ? TARGET_WORLD_WIDTH / (float)(max_x - min_x)
                             : 1.0f;
        layout.scale_z = max_y > min_y
                             ? TARGET_WORLD_DEPTH / (float)(max_y - min_y)
                             : 1.0f;
    }
    layout.min_x = (float)(min_x - layout.center_x) * layout.scale_x;
    layout.max_x = (float)(max_x - layout.center_x) * layout.scale_x;
    layout.min_z = (float)(min_y - layout.center_z) * layout.scale_z;
    layout.max_z = (float)(max_y - layout.center_z) * layout.scale_z;
    layout.overview_size =
        fmaxf(layout.max_z - layout.min_z, (layout.max_x - layout.min_x) / 1.34f) * 1.22f;
    return layout;
}

static Vector3 world_position(double x, double y, const SceneLayout *layout, float height) {
    return (Vector3){((float)x - layout->center_x) * layout->scale_x,
                     height,
                     ((float)y - layout->center_z) * layout->scale_z};
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

static float node_render_width(const Node *node, const SceneLayout *layout) {
    return fmaxf(node->width * layout->scale_x * BUILDING_SCALE, 0.24f);
}

static float node_render_depth(const Node *node, const SceneLayout *layout) {
    return fmaxf(node->depth * layout->scale_z * BUILDING_SCALE, 0.24f);
}

static int result_edge_direction(const PathResult *result, const Edge *edge) {
    int i;
    if (result == NULL || !result->found) return 0;
    for (i = 0; i + 1 < result->path_length; ++i) {
        if (result->path[i] == edge->from_id &&
            result->path[i + 1] == edge->to_id) return 1;
        if (result->path[i] == edge->to_id &&
            result->path[i + 1] == edge->from_id) return -1;
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

static const Place *selected_place(const PlaceStore *places, int place_id,
                                   int entrance_node_id) {
    const Place *place = storage_find_place(places, place_id);
    if (place != NULL && place->entrance_node_id == entrance_node_id) {
        return place;
    }
    return storage_find_place_by_entrance(places, entrance_node_id);
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
        case AREA_LAKE: return COLOR_WATER;
        case AREA_SPORTS: return (Color){157, 188, 151, 255};
        case AREA_SQUARE: return (Color){226, 218, 194, 255};
        case AREA_GRASS:
        default: return COLOR_GRASS;
    }
}

static void draw_soft_lake(Vector3 center, float width, float depth) {
    float radius = depth * 0.50f;
    float span = fmaxf(0.0f, width - radius * 2.0f);
    float offsets[] = {-span * 0.50f, 0.0f, span * 0.50f};
    int i;
    for (i = 0; i < 3; ++i) {
        DrawCylinder((Vector3){center.x + offsets[i], -0.010f, center.z},
                     radius + 0.07f, radius + 0.07f, 0.020f, 32, COLOR_WATER_BORDER);
    }
    for (i = 0; i < 3; ++i) {
        DrawCylinder((Vector3){center.x + offsets[i], -0.002f, center.z},
                     radius, radius, 0.022f, 32, COLOR_WATER);
    }
    DrawCube((Vector3){center.x - width * 0.12f, 0.018f, center.z - depth * 0.18f},
             width * 0.34f, 0.008f, 0.035f, Fade(WHITE, 0.52f));
    DrawCube((Vector3){center.x + width * 0.16f, 0.019f, center.z + depth * 0.20f},
             width * 0.22f, 0.008f, 0.028f, Fade(WHITE, 0.38f));
}

static void draw_sports_field(Vector3 center, float width, float depth) {
    DrawCube((Vector3){center.x, -0.010f, center.z}, width, 0.025f, depth,
             (Color){146, 185, 139, 255});
    DrawCubeWires((Vector3){center.x, -0.008f, center.z}, width, 0.028f, depth,
                  (Color){100, 137, 98, 210});
    DrawCubeWires((Vector3){center.x, -0.006f, center.z}, width * 0.82f,
                  0.030f, depth * 0.72f, (Color){244, 240, 219, 220});
    DrawCube((Vector3){center.x, -0.004f, center.z}, 0.025f, 0.032f,
             depth * 0.72f, Fade(WHITE, 0.72f));
}

static void draw_environment(const SceneLayout *layout) {
    int i;
    float ground_width = layout->max_x - layout->min_x + 5.5f;
    float ground_depth = layout->max_z - layout->min_z + 5.0f;
    DrawPlane((Vector3){0.0f, -0.035f, 0.0f}, (Vector2){ground_width, ground_depth},
              COLOR_MAP_GROUND);
    DrawCubeWires((Vector3){0.0f, -0.025f, 0.0f}, ground_width, 0.02f, ground_depth,
                  Fade((Color){118, 143, 121, 255}, 0.34f));
    if (!layout->legacy_decorations) return;
    for (i = 0; i < (int)(sizeof(MAP_AREAS) / sizeof(MAP_AREAS[0])); ++i) {
        const MapArea *area = &MAP_AREAS[i];
        Vector3 center = world_position(area->x, area->y, layout, -0.012f);
        float width = area->width * layout->scale_x;
        float depth = area->depth * layout->scale_z;
        if (area->type == AREA_LAKE) {
            draw_soft_lake(center, width, depth);
            continue;
        }
        DrawCube(center, width, 0.025f, depth, area_color(area->type));
        if (area->type != AREA_GRASS) {
            DrawCubeWires(center, width, 0.027f, depth,
                          area->type == AREA_LAKE ? COLOR_WATER_BORDER
                                                  : Fade((Color){102, 119, 105, 255}, 0.38f));
        }
        if (area->type == AREA_SPORTS) {
            DrawCubeWires(center, width * 0.82f, 0.029f, depth * 0.70f,
                          (Color){161, 102, 139, 220});
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

static void draw_flowering_tree(float x, float y, const SceneLayout *layout) {
    Vector3 base = world_position(x, y, layout, 0.0f);
    DrawCylinder((Vector3){base.x, 0.09f, base.z}, 0.030f, 0.040f, 0.18f, 8,
                 (Color){121, 91, 69, 255});
    DrawSphere((Vector3){base.x - 0.06f, 0.25f, base.z}, 0.13f,
               (Color){227, 166, 182, 255});
    DrawSphere((Vector3){base.x + 0.07f, 0.27f, base.z + 0.03f}, 0.12f,
               (Color){239, 185, 195, 255});
}

static void draw_shrub(float x, float y, const SceneLayout *layout, Color color) {
    Vector3 base = world_position(x, y, layout, 0.08f);
    DrawSphere(base, 0.09f, color);
    DrawSphere((Vector3){base.x + 0.10f, base.y, base.z + 0.03f}, 0.07f, color);
}

static void draw_trees(const SceneLayout *layout) {
    int i;
    if (!layout->legacy_decorations) return;
    for (i = 0; i < (int)(sizeof(TREE_POINTS) / sizeof(TREE_POINTS[0])); ++i) {
        if (i == 5 || i == 8 || i == 15 || i == 18) {
            draw_flowering_tree(TREE_POINTS[i].x, TREE_POINTS[i].y, layout);
        } else {
            draw_tree(TREE_POINTS[i].x, TREE_POINTS[i].y, layout);
        }
    }
    draw_shrub(5.9f, 6.4f, layout, (Color){105, 158, 94, 255});
    draw_shrub(6.4f, 5.1f, layout, (Color){116, 169, 103, 255});
    draw_shrub(7.8f, 5.1f, layout, (Color){202, 139, 160, 255});
    draw_shrub(8.0f, 6.4f, layout, (Color){108, 161, 96, 255});
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
    if (name_contains(node->name, "Canteen")) return (Color){204, 137, 78, 255};
    if (name_contains(node->name, "Dorm")) return (Color){190, 139, 96, 255};
    if (name_contains(node->name, "Hospital")) return (Color){210, 132, 127, 255};
    if (name_contains(node->name, "Library")) return (Color){60, 101, 148, 255};
    if (name_contains(node->name, "SecondBasic") ||
        name_contains(node->name, "EngineeringTraining")) return (Color){91, 126, 164, 255};
    if (name_contains(node->name, "Gym") || name_contains(node->name, "Sports"))
        return (Color){145, 108, 158, 255};
    if (name_contains(node->name, "College") || name_contains(node->name, "Teaching") ||
        name_contains(node->name, "Basic")) return (Color){118, 148, 169, 255};
    return (Color){148, 159, 153, 255};
}

static void draw_building_details(const Node *node, Vector3 center,
                                  float width, float depth, float height) {
    Color window = name_contains(node->name, "Canteen") || name_contains(node->name, "Dorm")
                       ? (Color){247, 220, 169, 235}
                       : (Color){205, 229, 238, 235};
    float facade_z = center.z + depth * 0.505f;
    if (height >= 0.17f && width >= 0.9f) {
        DrawCube((Vector3){center.x - width * 0.24f, center.y, facade_z},
                 width * 0.16f, height * 0.24f, 0.025f, window);
        DrawCube((Vector3){center.x + width * 0.24f, center.y, facade_z},
                 width * 0.16f, height * 0.24f, 0.025f, window);
    }
    DrawCube((Vector3){center.x, 0.045f, center.z + depth * 0.54f},
             fminf(width * 0.22f, 0.38f), 0.09f, 0.10f,
             (Color){76, 91, 87, 225});
    DrawCube((Vector3){center.x, 0.018f, center.z + depth * 0.64f},
             fminf(width * 0.34f, 0.52f), 0.025f, 0.16f,
             (Color){207, 205, 190, 255});
}

static void draw_gate(const Node *node, const SceneLayout *layout, Color color) {
    Vector3 center = node_position(node, layout, 0.0f);
    float width = fmaxf(node_render_width(node, layout), 0.95f);
    float height = node_render_height(node);
    float depth = fmaxf(node_render_depth(node, layout), 0.38f);
    Vector3 left = {center.x - width * 0.40f, height * 0.5f, center.z};
    Vector3 right = {center.x + width * 0.40f, height * 0.5f, center.z};
    DrawCube(left, width * 0.14f, height, depth, color);
    DrawCube(right, width * 0.14f, height, depth, color);
    DrawCube((Vector3){center.x, height, center.z}, width, height * 0.22f, depth, color);
}

static void draw_node(const Node *node, const SceneLayout *layout,
                      const RendererState *state) {
    Vector3 center = node_position(node, layout, 0.0f);
    float width = node_render_width(node, layout);
    float depth = node_render_depth(node, layout);
    float height = node_render_height(node);
    Color color;
    int highlighted = node->id == state->hover_id || node->id == state->search_match_id;
    if (node->type == NODE_JUNCTION || node->type == NODE_BRIDGE ||
        node->type == NODE_ENTRANCE || node->type == NODE_SERVICE) return;
    if (!layout->legacy_decorations && !node->visible) return;
    if (node->type == NODE_LAKE) {
        if (!layout->legacy_decorations) draw_soft_lake(center, width, depth);
        return;
    }
    if (node->type == NODE_SQUARE) {
        if (!layout->legacy_decorations) draw_sports_field(center, width, depth);
        return;
    }
    if (layout->legacy_decorations && node->id == 4) return;
    if (layout->legacy_decorations && node->id == 16) {
        DrawCube((Vector3){center.x, 0.012f, center.z}, width, 0.024f, depth,
                 (Color){98, 174, 211, 255});
        DrawCubeWires((Vector3){center.x, 0.014f, center.z}, width, 0.026f, depth,
                      (Color){55, 119, 159, 190});
        return;
    }
    color = building_color(node);
    if (highlighted) color = ColorBrightness(color, 0.18f);
    if (node->type == NODE_GATE) {
        draw_gate(node, layout, (Color){67, 145, 99, 255});
        return;
    }
    DrawCube((Vector3){center.x + 0.10f, 0.020f, center.z + 0.10f},
             width + 0.05f, 0.022f, depth + 0.05f, Fade((Color){54, 67, 63, 255}, 0.25f));
    center.y = height * 0.5f;
    DrawCube(center, width, height, depth, color);
    DrawCubeWires(center, width, height, depth, Fade((Color){58, 72, 72, 255}, 0.52f));
    if (highlighted) {
        DrawCubeWires(center, width + 0.08f, height + 0.06f, depth + 0.08f,
                      node->id == state->search_match_id
                          ? (Color){239, 151, 52, 255}
                          : (Color){51, 127, 105, 255});
    }
    DrawCube((Vector3){center.x, height + 0.012f, center.z},
             width * 0.88f, 0.025f, depth * 0.88f, Fade(RAYWHITE, 0.24f));
    draw_building_details(node, center, width, depth, height);
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

static void draw_route_arrow(Vector3 from, Vector3 to) {
    float dx = to.x - from.x;
    float dz = to.z - from.z;
    float length = sqrtf(dx * dx + dz * dz);
    Vector3 middle, tip, left, right;
    float px, pz;
    if (length < 1.2f) return;
    dx /= length;
    dz /= length;
    px = -dz;
    pz = dx;
    middle = Vector3Lerp(from, to, 0.53f);
    middle.y = PATH_HEIGHT + 0.018f;
    tip = (Vector3){middle.x + dx * 0.24f, middle.y, middle.z + dz * 0.24f};
    left = (Vector3){middle.x - dx * 0.14f + px * 0.13f, middle.y,
                     middle.z - dz * 0.14f + pz * 0.13f};
    right = (Vector3){middle.x - dx * 0.14f - px * 0.13f, middle.y,
                      middle.z - dz * 0.14f - pz * 0.13f};
    DrawTriangle3D(left, tip, right, (Color){255, 239, 195, 245});
}

static int road_class(const Edge *edge) {
    if (edge->type == ROAD_MAIN || edge->type == ROAD_BRIDGE) return 2;
    if (edge->type == ROAD_BRANCH) return 1;
    return 0;
}

static float road_width(RoadType type) {
    if (type == ROAD_MAIN || type == ROAD_BRIDGE) return 0.42f;
    if (type == ROAD_BRANCH) return 0.27f;
    return 0.13f;
}

static Color road_fill(RoadType type) {
    if (type == ROAD_BRIDGE) return COLOR_ROAD_BRIDGE;
    if (type == ROAD_MAIN) return COLOR_ROAD_MAIN;
    if (type == ROAD_BRANCH) return COLOR_ROAD_BRANCH;
    return COLOR_ROAD_ENTRANCE;
}

static void draw_road_disc(Vector3 position, float width, float height, Color color) {
    DrawCylinder((Vector3){position.x, height, position.z}, width * 0.5f, width * 0.5f,
                 0.008f, 18, color);
}

static int edge_has_geometry(const Graph *graph, const Edge *edge) {
    return edge->geometry_count >= 2 && edge->geometry_start >= 0 &&
           edge->geometry_start <= graph->geometry_point_count &&
           edge->geometry_count <= graph->geometry_point_count - edge->geometry_start;
}

static int edge_render_point_count(const Graph *graph, const Edge *edge) {
    return edge_has_geometry(graph, edge) ? edge->geometry_count : 2;
}

static Vector3 edge_render_point(const Graph *graph, const Edge *edge, int index,
                                 int reverse, const SceneLayout *layout,
                                 float height) {
    if (edge_has_geometry(graph, edge)) {
        int point_index = reverse ? edge->geometry_count - index - 1 : index;
        const MapPoint *point =
            &graph->geometry_points[edge->geometry_start + point_index];
        return world_position(point->x, point->y, layout, height);
    }
    {
        int node_id = reverse
                          ? (index == 0 ? edge->to_id : edge->from_id)
                          : (index == 0 ? edge->from_id : edge->to_id);
        const Node *node = graph_get_node(graph, node_id);
        return node != NULL ? node_position(node, layout, height)
                            : (Vector3){0.0f, height, 0.0f};
    }
}

static void draw_roads(const Graph *graph, const SceneLayout *layout,
                       const RendererState *state) {
    int i;
    int pass;
    int show_path = state->has_result &&
                    state->animation_count >= state->result.visited_count;
    for (pass = 0; pass <= 2; ++pass) {
        for (i = 0; i < graph->edge_count; ++i) {
            const Edge *edge = &graph->edges[i];
            const Node *from = graph_get_node(graph, edge->from_id);
            const Node *to = graph_get_node(graph, edge->to_id);
            int point_count;
            int point_index;
            float width;
            Color border;
            if (!edge->walkable || from == NULL || to == NULL || road_class(edge) != pass) continue;
            width = road_width(edge->type);
            border = pass == 0 ? Fade(COLOR_ROAD_BORDER, 0.50f) : COLOR_ROAD_BORDER;
            point_count = edge_render_point_count(graph, edge);
            for (point_index = 0; point_index + 1 < point_count; ++point_index) {
                Vector3 a = edge_render_point(graph, edge, point_index, 0,
                                              layout, ROAD_HEIGHT);
                Vector3 b = edge_render_point(graph, edge, point_index + 1, 0,
                                              layout, ROAD_HEIGHT);
                draw_band(a, b, width + 0.08f, border);
                draw_road_disc(a, width + 0.08f, ROAD_HEIGHT, border);
                draw_road_disc(b, width + 0.08f, ROAD_HEIGHT, border);
                a.y += 0.006f;
                b.y += 0.006f;
                draw_band(a, b, width, road_fill(edge->type));
                draw_road_disc(a, width, ROAD_HEIGHT + 0.006f,
                               road_fill(edge->type));
                draw_road_disc(b, width, ROAD_HEIGHT + 0.006f,
                               road_fill(edge->type));
            }
        }
    }
    if (!show_path) return;
    for (i = 0; i < graph->edge_count; ++i) {
        const Edge *edge = &graph->edges[i];
        int direction = result_edge_direction(&state->result, edge);
        int point_count;
        int point_index;
        Vector3 arrow_from = {0};
        Vector3 arrow_to = {0};
        float longest_segment = 0.0f;
        if (!edge->walkable || direction == 0 ||
            graph_get_node(graph, edge->from_id) == NULL ||
            graph_get_node(graph, edge->to_id) == NULL) continue;
        point_count = edge_render_point_count(graph, edge);
        for (point_index = 0; point_index + 1 < point_count; ++point_index) {
            Vector3 a = edge_render_point(graph, edge, point_index, direction < 0,
                                          layout, PATH_HEIGHT);
            Vector3 b = edge_render_point(graph, edge, point_index + 1, direction < 0,
                                          layout, PATH_HEIGHT);
            Vector3 shadow_a = a;
            Vector3 shadow_b = b;
            float dx = b.x - a.x;
            float dz = b.z - a.z;
            float segment_length = sqrtf(dx * dx + dz * dz);
            shadow_a.y = shadow_b.y = PATH_HEIGHT - 0.009f;
            draw_band(shadow_a, shadow_b, 0.84f, Fade(COLOR_ROUTE, 0.20f));
            shadow_a.y = shadow_b.y = PATH_HEIGHT - 0.004f;
            draw_band(shadow_a, shadow_b, 0.62f, COLOR_ROUTE_SHADOW);
            draw_road_disc(shadow_a, 0.62f, PATH_HEIGHT - 0.004f,
                           COLOR_ROUTE_SHADOW);
            draw_road_disc(shadow_b, 0.62f, PATH_HEIGHT - 0.004f,
                           COLOR_ROUTE_SHADOW);
            draw_band(a, b, 0.46f, COLOR_ROUTE);
            draw_road_disc(a, 0.46f, PATH_HEIGHT, COLOR_ROUTE);
            draw_road_disc(b, 0.46f, PATH_HEIGHT, COLOR_ROUTE);
            draw_road_disc(a, 0.13f, PATH_HEIGHT + 0.010f, WHITE);
            draw_road_disc(b, 0.13f, PATH_HEIGHT + 0.010f, WHITE);
            if (segment_length > longest_segment) {
                longest_segment = segment_length;
                arrow_from = a;
                arrow_to = b;
            }
        }
        if (longest_segment > 0.0f) draw_route_arrow(arrow_from, arrow_to);
    }
}

static void draw_map_pin(const Node *node, const SceneLayout *layout, Color color) {
    Vector3 position = node_position(node, layout, 0.0f);
    DrawCylinder((Vector3){position.x, 0.12f, position.z}, 0.24f, 0.24f,
                 0.055f, 24, WHITE);
    DrawCylinder((Vector3){position.x, 0.155f, position.z}, 0.17f, 0.17f,
                 0.065f, 24, color);
    DrawSphere((Vector3){position.x, 0.30f, position.z}, 0.105f, color);
}

static void draw_selection_markers(const Graph *graph, const SceneLayout *layout,
                                   const RendererState *state) {
    const Node *start = graph_get_node(graph, state->start_id);
    const Node *goal = graph_get_node(graph, state->goal_id);
    const Node *search = graph_get_node(graph, state->search_match_id);
    const Node *hover = graph_get_node(graph, state->hover_id);
    if (start != NULL) draw_map_pin(start, layout, COLOR_START);
    if (goal != NULL) draw_map_pin(goal, layout, COLOR_GOAL);
    if (search != NULL && search->id != state->start_id && search->id != state->goal_id) {
        Vector3 position = node_position(search, layout, 0.10f);
        DrawCylinder(position, 0.25f, 0.25f, 0.025f, 24,
                     Fade((Color){242, 158, 57, 255}, 0.86f));
    }
    if (hover != NULL && hover->id != state->start_id && hover->id != state->goal_id &&
        hover->id != state->search_match_id) {
        Vector3 position = node_position(hover, layout, 0.09f);
        DrawCylinder(position, 0.23f, 0.23f, 0.022f, 24,
                     Fade(COLOR_ACCENT, 0.78f));
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

static int pick_node(const Graph *graph, const PlaceStore *places,
                     const SceneLayout *layout, Ray ray) {
    int i, picked = -1;
    float nearest = 1.0e30f;
    for (i = 0; i < graph->node_count; ++i) {
        const Node *node = &graph->nodes[i];
        Vector3 center, half;
        BoundingBox box;
        RayCollision hit;
        float height;
        if (storage_find_place_by_entrance(places, node->id) == NULL) continue;
        height = fmaxf(node_render_height(node), 0.08f);
        center = node_position(node, layout, height * 0.5f);
        half = (Vector3){fmaxf(node_render_width(node, layout) * 0.55f, 0.32f),
                         fmaxf(height, 0.18f),
                         fmaxf(node_render_depth(node, layout) * 0.55f, 0.32f)};
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
    return id == 0 || id == 2 || id == 4 || id == 5 || id == 7 || id == 11 || id == 12 ||
           id == 13 || id == 14 || id == 15 || id == 18 || id == 24 || id == 25;
}

static int label_visible(const Node *node, const Place *place, const Camera3D *camera,
                         const SceneLayout *layout, const RendererState *state) {
    if (node->id == state->start_id || node->id == state->goal_id ||
        node->id == state->hover_id || node->id == state->search_match_id) return 1;
    if (place != NULL && place->name[0] != '\0') return 1;
    if (!node->visible) return 0;
    if (layout->legacy_decorations && is_important_landmark(node->id)) return 1;
    return camera->projection == CAMERA_ORTHOGRAPHIC &&
           camera->fovy < layout->overview_size * 0.58f;
}

static void draw_rounded_label(Rectangle bounds, const char *text, int font_size,
                               int priority) {
    Rectangle shadow = {bounds.x + 2.0f, bounds.y + 2.0f, bounds.width, bounds.height};
    DrawRectangleRounded(shadow, 0.24f, 6, Fade((Color){56, 69, 64, 255}, 0.16f));
    DrawRectangleRounded(bounds, 0.24f, 6,
                         priority ? (Color){255, 255, 253, 242}
                                  : (Color){255, 255, 253, 220});
    DrawRectangleRoundedLinesEx(bounds, 0.24f, 6, 1.0f,
                                Fade((Color){101, 116, 109, 255}, 0.42f));
    DrawText(text, (int)bounds.x + 5, (int)bounds.y + 3, font_size, COLOR_TEXT);
}

static void draw_labels(const Graph *graph, const PlaceStore *places,
                        const SceneLayout *layout,
                        const Camera3D *camera, const RendererState *state) {
    Rectangle occupied[MSP_MAX_NODES];
    int occupied_count = 0;
    int pass, i;
    for (pass = 0; pass < 2; ++pass) {
        for (i = 0; i < graph->node_count; ++i) {
            const Node *node = &graph->nodes[i];
            int selected_priority = node->id == state->start_id ||
                                    node->id == state->goal_id ||
                                    node->id == state->hover_id ||
                                    node->id == state->search_match_id;
            Vector2 screen;
            Rectangle bounds;
            int j, collision = 0;
            const Place *place = node->id == state->start_id
                                     ? selected_place(places, state->start_place_id, node->id)
                                 : node->id == state->goal_id
                                     ? selected_place(places, state->goal_place_id, node->id)
                                 : node->id == state->search_match_id
                                     ? selected_place(places, state->search_match_place_id, node->id)
                                     : storage_find_place_by_entrance(places, node->id);
            int priority = selected_priority || place != NULL;
            int font_size = selected_priority ? 15 : 13;
            const char *label = place != NULL && place->name[0] != '\0'
                                    ? place->name
                                    : node->name;
            if (priority != (pass == 0) ||
                !label_visible(node, place, camera, layout, state)) continue;
            screen = GetWorldToScreenEx(
                node_position(node, layout, node_render_height(node) + 0.17f),
                *camera, MAP_WIDTH, MAP_HEIGHT);
            if (screen.x < 5 || screen.x > MAP_WIDTH - 5 ||
                screen.y < 5 || screen.y > MAP_HEIGHT - 5) continue;
            bounds = (Rectangle){screen.x - MeasureText(label, font_size) * 0.5f - 5.0f,
                                 screen.y - font_size - 6.0f,
                                 (float)MeasureText(label, font_size) + 10.0f,
                                 (float)font_size + 7.0f};
            for (j = 0; j < occupied_count; ++j) {
                if (CheckCollisionRecs(bounds, occupied[j])) { collision = 1; break; }
            }
            if (collision && !priority) continue;
            draw_rounded_label(bounds, label, font_size, priority);
            if (occupied_count < MSP_MAX_NODES) occupied[occupied_count++] = bounds;
        }
    }
}

static void draw_screen_pin(const Node *node, const SceneLayout *layout,
                            const Camera3D *camera, Color color, const char *marker) {
    Vector2 screen;
    if (node == NULL) return;
    screen = GetWorldToScreenEx(node_position(node, layout, 0.34f),
                                *camera, MAP_WIDTH, MAP_HEIGHT);
    if (screen.x < 12 || screen.x > MAP_WIDTH - 12 ||
        screen.y < 12 || screen.y > MAP_HEIGHT - 12) return;
    DrawTriangle((Vector2){screen.x, screen.y + 18},
                 (Vector2){screen.x - 6, screen.y + 6},
                 (Vector2){screen.x + 6, screen.y + 6}, color);
    DrawCircle((int)screen.x + 1, (int)screen.y + 2, 11,
               Fade((Color){45, 55, 51, 255}, 0.18f));
    DrawCircle((int)screen.x, (int)screen.y, 11, WHITE);
    DrawCircle((int)screen.x, (int)screen.y, 8, color);
    DrawText(marker, (int)screen.x - MeasureText(marker, 10) / 2,
             (int)screen.y - 5, 10, WHITE);
}

static void draw_screen_selection_markers(const Graph *graph, const SceneLayout *layout,
                                          const Camera3D *camera,
                                          const RendererState *state) {
    draw_screen_pin(graph_get_node(graph, state->start_id), layout, camera, COLOR_START, "S");
    draw_screen_pin(graph_get_node(graph, state->goal_id), layout, camera, COLOR_GOAL, "D");
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

static Rectangle legend_panel(void) {
    return (Rectangle){WINDOW_WIDTH - 166.0f, 18.0f, 146.0f, 166.0f};
}

static void draw_ui_card(Rectangle bounds, Color fill, Color border) {
    Rectangle shadow = {bounds.x + 2.0f, bounds.y + 3.0f, bounds.width, bounds.height};
    DrawRectangleRounded(shadow, 0.14f, 6, Fade((Color){55, 67, 62, 255}, 0.10f));
    DrawRectangleRounded(bounds, 0.14f, 6, fill);
    DrawRectangleRoundedLinesEx(bounds, 0.14f, 6, 1.0f, border);
}

static void draw_gradient_button(Rectangle bounds, int hovered) {
    Color left = hovered ? (Color){80, 104, 205, 255} : (Color){72, 91, 190, 255};
    Color right = hovered ? (Color){126, 91, 199, 255} : (Color){110, 76, 184, 255};
    int x;
    DrawRectangleRounded((Rectangle){bounds.x + 2, bounds.y + 4, bounds.width, bounds.height},
                         0.20f, 7, Fade((Color){48, 48, 86, 255}, 0.20f));
    DrawRectangleRounded(bounds, 0.20f, 7, left);
    for (x = 6; x < (int)bounds.width - 6; ++x) {
        float factor = (float)x / bounds.width;
        DrawLine((int)bounds.x + x, (int)bounds.y + 4,
                 (int)bounds.x + x, (int)(bounds.y + bounds.height - 5),
                 ColorLerp(left, right, factor));
    }
    DrawRectangleRoundedLinesEx(bounds, 0.20f, 7, 1.0f, Fade(WHITE, 0.28f));
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

static void draw_navigation_panel(const Graph *graph, const PlaceStore *places,
                                  const RendererState *state) {
    const Node *start = graph_get_node(graph, state->start_id);
    const Node *goal = graph_get_node(graph, state->goal_id);
    const Place *start_place = selected_place(places, state->start_place_id, state->start_id);
    const Place *goal_place = selected_place(places, state->goal_place_id, state->goal_id);
    int y;
    DrawRectangle(0, 0, NAV_WIDTH, WINDOW_HEIGHT, COLOR_MAP_SKY);
    draw_ui_card((Rectangle){10, 10, NAV_WIDTH - 20, WINDOW_HEIGHT - 20},
                 Fade(COLOR_PANEL, 0.98f), (Color){218, 226, 220, 255});
    DrawText("SCU JIANG'AN", 20, 20, 13, (Color){76, 116, 100, 255});
    DrawText("Campus Navigation", 20, 42, 24, (Color){33, 66, 57, 255});
    DrawText("START", 20, 91, 11, (Color){98, 111, 105, 255});
    draw_ui_card((Rectangle){20, 108, 260, 48}, (Color){241, 248, 242, 255},
                 (Color){211, 226, 214, 255});
    DrawCircle(38, 132, 6, (Color){48, 170, 99, 255});
    draw_text_fit(start ? (start_place ? start_place->name : start->name)
                        : "Click a place on the map", 53, 123, 215, 15,
                  (Color){44, 63, 57, 255});
    DrawText("DESTINATION", 20, 169, 11, (Color){98, 111, 105, 255});
    draw_ui_card((Rectangle){20, 186, 260, 48}, (Color){252, 242, 239, 255},
                 (Color){235, 216, 211, 255});
    DrawCircle(38, 210, 6, (Color){221, 78, 69, 255});
    draw_text_fit(goal ? (goal_place ? goal_place->name : goal->name)
                       : "Right-click a destination", 53, 201, 215, 15,
                  (Color){44, 63, 57, 255});
    DrawText("ROUTE MODE", 20, 241, 11, (Color){98, 111, 105, 255});
    {
        int algorithm;
        for (algorithm = 1; algorithm <= 2; ++algorithm) {
            Rectangle button = algorithm_button(algorithm);
            int active = state->algorithm == algorithm;
            int hovered = CheckCollisionPointRec(GetMousePosition(), button);
            Color idle = hovered ? (Color){231, 237, 232, 255}
                                 : (Color){238, 241, 237, 255};
            draw_ui_card(button, active ? COLOR_ACCENT : idle,
                         active ? COLOR_ACCENT : (Color){220, 226, 220, 255});
            DrawText(algorithm == 1 ? "Dijkstra" : "A*",
                     (int)button.x + (algorithm == 1 ? 31 : 52), (int)button.y + 9,
                     14, active ? RAYWHITE : (Color){69, 82, 77, 255});
        }
    }
    draw_gradient_button(navigate_button(),
                         CheckCollisionPointRec(GetMousePosition(), navigate_button()));
    DrawText("Start navigation", 87, 322, 16, RAYWHITE);
    DrawText(state->message, 20, 367, 13, (Color){77, 99, 90, 255});
    DrawLine(20, 396, 280, 396, (Color){218, 224, 217, 255});
    DrawText("ROUTE DETAILS", 20, 414, 11, (Color){98, 111, 105, 255});
    if (!state->has_result) {
        DrawText("Select two places to see", 20, 442, 15, (Color){119, 128, 124, 255});
        DrawText("distance and walking time.", 20, 463, 15, (Color){119, 128, 124, 255});
        return;
    }
    {
        char distance_text[32];
        const char *distance_unit = graph->weights_in_meters ? "m" : "distance units";
        snprintf(distance_text, sizeof(distance_text), "%.1f",
                 state->result.total_distance);
        DrawText(distance_text, 20, 440, 30, (Color){42, 72, 62, 255});
        DrawText(distance_unit, 26 + MeasureText(distance_text, 30), 451, 13,
                 (Color){111, 123, 117, 255});
        if (graph->weights_in_meters) {
            DrawText(TextFormat("About %.1f min walk",
                                state->result.total_distance / 80.0),
                     20, 480, 15, (Color){70, 84, 79, 255});
        } else {
            DrawText(TextFormat("About %.0f min walk",
                                fmax(1.0, state->result.total_distance * 1.25)),
                     20, 480, 15, (Color){70, 84, 79, 255});
        }
    }
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

static void draw_search_ui(const Graph *graph, const PlaceStore *places,
                           const RendererState *state) {
    Rectangle box = search_box();
    const Node *match = graph_get_node(graph, state->search_match_id);
    const Place *match_place = selected_place(places, state->search_match_place_id,
                                              state->search_match_id);
    DrawRectangleRounded((Rectangle){box.x + 2, box.y + 3, box.width, box.height},
                         0.15f, 6, Fade((Color){49, 66, 59, 255}, 0.13f));
    DrawRectangleRounded(box, 0.15f, 6, (Color){255, 255, 253, 248});
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
        draw_ui_card(result, COLOR_CARD, (Color){210, 220, 214, 255});
        DrawText(TextFormat("[%02d] %s", match_place ? match_place->id : match->id,
                            match_place ? match_place->name : match->name),
                 (int)result.x + 12, (int)result.y + 10, 14, (Color){49, 72, 62, 255});
    }
}

static void draw_legend(void) {
    Rectangle panel = legend_panel();
    int x = (int)panel.x;
    int y = (int)panel.y;
    const char *labels[] = {"Teaching", "Dorm / dining", "Green space", "Water", "Road", "Route"};
    Color colors[] = {{118,148,169,255}, {204,137,78,255}, {195,216,184,255},
                      {112,188,224,255}, {244,245,239,255}, {239,126,40,255}};
    int i;
    DrawRectangleRounded((Rectangle){panel.x + 2, panel.y + 3, panel.width, panel.height},
                         0.10f, 6, Fade((Color){52, 64, 60, 255}, 0.12f));
    DrawRectangleRounded(panel, 0.10f, 6, (Color){255, 255, 253, 224});
    DrawRectangleRoundedLinesEx(panel, 0.10f, 6, 1.0f,
                                Fade((Color){115, 129, 122, 255}, 0.32f));
    DrawText("MAP LEGEND", x + 12, y + 12, 11, (Color){73, 91, 83, 255});
    for (i = 0; i < 6; ++i) {
        DrawRectangle(x + 13, y + 38 + i * 20, 14, 8, colors[i]);
        DrawText(labels[i], x + 35, y + 34 + i * 20, 12, (Color){58, 70, 66, 255});
    }
}

static void draw_tool_button(int index, const char *label, int active) {
    Rectangle button = tool_button(index);
    int hovered = CheckCollisionPointRec(GetMousePosition(), button);
    DrawRectangleRounded((Rectangle){button.x + 2, button.y + 2, button.width, button.height},
                         0.16f, 5, Fade((Color){45, 58, 53, 255}, 0.13f));
    DrawRectangleRounded(button, 0.16f, 5,
                         active ? (Color){57, 111, 94, 255}
                                : hovered ? (Color){239, 245, 241, 250}
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

static int update_search(const PlaceStore *places, RendererState *state) {
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
        state->search_match_place_id = -1;
        if (state->search_query[0]) {
            for (i = 0; places != NULL && i < places->place_count; ++i) {
                const Place *place = &places->places[i];
                if (name_contains(place->name, state->search_query) ||
                    name_contains(place->alias, state->search_query)) {
                    state->search_match_id = place->entrance_node_id;
                    state->search_match_place_id = place->id;
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

void renderer3d_run(const Graph *graph, const PlaceStore *places) {
    SceneLayout layout;
    Camera3D camera;
    RendererState state = {0};
    RenderTexture2D map_target;
    RenderTexture2D screen_target = {0};
    int screenshot_mode = getenv("MSP_3D_SCREENSHOT") != NULL;
    int frame_count = 0;
    if (graph == NULL || graph->node_count <= 0 || places == NULL ||
        places->place_count <= 0) return;
    layout = build_scene_layout(graph);
    state.start_id = -1;
    state.goal_id = -1;
    state.start_place_id = -1;
    state.goal_place_id = -1;
    state.hover_id = -1;
    state.search_match_id = -1;
    state.search_match_place_id = -1;
    state.algorithm = 1;
    state.display_3d = 1;
    state.animation_step = 0.18f;
    state.camera_view = VIEW_NAVIGATION;
    snprintf(state.message, sizeof(state.message), "Choose your route");
    camera = camera_for_view(graph, &layout, &state, state.camera_view);

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "SCU Jiang'an Campus Navigation");
    map_target = LoadRenderTexture(MAP_WIDTH, MAP_HEIGHT);
    if (screenshot_mode) screen_target = LoadRenderTexture(WINDOW_WIDTH, WINDOW_HEIGHT);
    SetTargetFPS(60);
    if (screenshot_mode) {
        const Place *start_place = &places->places[0];
        const Place *goal_place = &places->places[places->place_count - 1];
        state.start_id = start_place->entrance_node_id;
        state.goal_id = goal_place->entrance_node_id;
        state.start_place_id = start_place->id;
        state.goal_place_id = goal_place->id;
        run_search(graph, &state);
        state.animation_count = state.result.visited_count;
        snprintf(state.message, sizeof(state.message), "Recommended route ready");
        state.camera_view = VIEW_OVERVIEW;
        camera = camera_for_view(graph, &layout, &state, state.camera_view);
    }

    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();
        int over_map = mouse.x >= NAV_WIDTH && mouse.x < WINDOW_WIDTH;
        int map_ui_hit = CheckCollisionPointRec(mouse, search_box()) ||
                         CheckCollisionPointRec(mouse, legend_panel());
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

        update_search(places, &state);
        if (state.search_focus && IsKeyPressed(KEY_ENTER) && state.search_match_id >= 0) {
            const Node *match = graph_get_node(graph, state.search_match_id);
            if (state.start_id < 0) {
                state.start_id = state.search_match_id;
                state.start_place_id = state.search_match_place_id;
            } else {
                state.goal_id = state.search_match_id;
                state.goal_place_id = state.search_match_place_id;
            }
            if (match != NULL) {
                Vector3 target = node_position(match, &layout, 0.0f);
                Vector3 offset = Vector3Subtract(camera.position, camera.target);
                camera.target = target;
                camera.position = Vector3Add(target, offset);
                if (camera.projection == CAMERA_ORTHOGRAPHIC) {
                    camera.fovy = fminf(camera.fovy, layout.overview_size * 0.52f);
                }
            }
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
            state.start_place_id = state.goal_place_id = -1;
            state.has_result = 0;
            snprintf(state.message, sizeof(state.message), "Route cleared");
        }
        if (camera_changed) camera = camera_for_view(graph, &layout, &state, state.camera_view);

        state.hover_id = -1;
        if (over_map && !map_ui_hit) {
            Vector2 local_mouse = {mouse.x - NAV_WIDTH, mouse.y};
            state.hover_id = pick_node(graph, places, &layout,
                GetScreenToWorldRayEx(local_mouse, camera, MAP_WIDTH, MAP_HEIGHT));
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
                IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                picked = state.hover_id;
                if (picked >= 0) {
                    const Place *place = storage_find_place_by_entrance(places, picked);
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        state.start_id = picked;
                        state.start_place_id = place != NULL ? place->id : -1;
                    }
                    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                        state.goal_id = picked;
                        state.goal_place_id = place != NULL ? place->id : -1;
                    }
                    state.has_result = 0;
                    snprintf(state.message, sizeof(state.message), "Press Start navigation");
                }
            }
        }
        update_camera_controls(&camera, &layout, over_map && !map_ui_hit);
        update_animation(&state);

        BeginTextureMode(map_target);
        ClearBackground(COLOR_MAP_SKY);
        BeginMode3D(camera);
        draw_environment(&layout);
        draw_roads(graph, &layout, &state);
        for (i = 0; i < graph->node_count; ++i) draw_node(&graph->nodes[i], &layout, &state);
        draw_trees(&layout);
        draw_search_markers(graph, &layout, &state);
        draw_selection_markers(graph, &layout, &state);
        EndMode3D();
        draw_labels(graph, places, &layout, &camera, &state);
        draw_screen_selection_markers(graph, &layout, &camera, &state);
        EndTextureMode();

        if (screenshot_mode) BeginTextureMode(screen_target);
        else BeginDrawing();
        ClearBackground(COLOR_MAP_SKY);
        DrawTextureRec(map_target.texture,
                       (Rectangle){0, 0, (float)MAP_WIDTH, -(float)MAP_HEIGHT},
                       (Vector2){NAV_WIDTH, 0}, WHITE);
        draw_navigation_panel(graph, places, &state);
        draw_search_ui(graph, places, &state);
        draw_legend();
        draw_map_tools(&state);
        if (screenshot_mode) {
            EndTextureMode();
            BeginDrawing();
            ClearBackground(BLACK);
            DrawTextureRec(screen_target.texture,
                           (Rectangle){0, 0, (float)WINDOW_WIDTH, -(float)WINDOW_HEIGHT},
                           (Vector2){0, 0}, WHITE);
            EndDrawing();
        } else {
            EndDrawing();
        }

        frame_count++;
        if (screenshot_mode && frame_count == 12) {
            Image screenshot = LoadImageFromTexture(screen_target.texture);
            ImageFlipVertical(&screenshot);
            ExportImage(screenshot, "3d-preview.png");
            UnloadImage(screenshot);
            break;
        }
    }
    if (screenshot_mode) UnloadRenderTexture(screen_target);
    UnloadRenderTexture(map_target);
    CloseWindow();
}
