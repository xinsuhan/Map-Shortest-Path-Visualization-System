#ifndef MAP_SHORTEST_PATH_ROAD_EDITOR_H
#define MAP_SHORTEST_PATH_ROAD_EDITOR_H

#include "models.h"
#include "map_transform.h"
#include "raylib.h"

#define ROAD_EDITOR_MAX_NODES 512
#define ROAD_EDITOR_MAX_EDGES 1024
#define ROAD_EDITOR_MAX_POINTS 8192
#define ROAD_EDITOR_SNAP_RADIUS 12.0f

typedef struct {
    int id;
    char name[MSP_NAME_LENGTH];
    float map_x;
    float map_y;
    NodeType type;
} EditorNode;

typedef struct {
    int id;
    int from_id;
    int to_id;
    RoadType type;
    int walkable;
    int point_start;
    int point_count;
    double weight;
} EditorEdge;

typedef struct {
    int active;
    int debug_visible;
    int node_count;
    int edge_count;
    int point_count;
    EditorNode nodes[ROAD_EDITOR_MAX_NODES];
    EditorEdge edges[ROAD_EDITOR_MAX_EDGES];
    MapPoint points[ROAD_EDITOR_MAX_POINTS];
    MapPoint current_points[MSP_MAX_GEOMETRY_POINTS];
    int current_point_count;
    int current_from_id;
    int snap_node_id;
    MapPoint mouse_map;
    int mouse_valid;
    char message[160];
} RoadEditor;

int world_to_map(Vector3 world, const MapTransform *transform, MapPoint *point);
void road_editor_init(RoadEditor *editor);
void road_editor_toggle(RoadEditor *editor);
void road_editor_update_cursor(RoadEditor *editor, MapPoint point, int valid);
void road_editor_add_node(RoadEditor *editor);
void road_editor_add_control_point(RoadEditor *editor);
void road_editor_undo(RoadEditor *editor);
void road_editor_cancel(RoadEditor *editor);
int road_editor_finish_edge(RoadEditor *editor);
int road_editor_save(const RoadEditor *editor, const char *directory);
void road_editor_draw_world(const RoadEditor *editor, const MapTransform *transform);
void road_editor_draw_overlay(const RoadEditor *editor, Font font);

#endif
