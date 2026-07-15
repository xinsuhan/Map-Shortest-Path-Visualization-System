#include "road_editor.h"

#include "raymath.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

int world_to_map(Vector3 world, const MapTransform *t, MapPoint *point) {
    if (t == NULL || point == NULL || fabsf(t->scale_x) < 1e-6f ||
        fabsf(t->scale_y) < 1e-6f) return 0;
    point->x = (world.x - t->offset_x) / t->scale_x;
    point->y = (world.z - t->offset_y) / t->scale_y;
    if (t->flip_y) point->y = -point->y;
    return 1;
}

static int nearest_node(const RoadEditor *e, MapPoint p) {
    int best = -1, i;
    double best_d = ROAD_EDITOR_SNAP_RADIUS * ROAD_EDITOR_SNAP_RADIUS;
    for (i = 0; i < e->node_count; ++i) {
        double dx = e->nodes[i].map_x - p.x, dy = e->nodes[i].map_y - p.y;
        double d = dx * dx + dy * dy;
        if (d <= best_d) { best_d = d; best = i; }
    }
    return best;
}

static MapPoint snapped_point(const RoadEditor *e) {
    if (e->snap_node_id >= 0) {
        const EditorNode *n = &e->nodes[e->snap_node_id];
        return (MapPoint){n->map_x, n->map_y};
    }
    return e->mouse_map;
}

void road_editor_init(RoadEditor *e) {
    memset(e, 0, sizeof(*e));
    e->current_from_id = -1;
    e->snap_node_id = -1;
    snprintf(e->message, sizeof(e->message), "F2: road editor");
}

void road_editor_toggle(RoadEditor *e) {
    e->active = !e->active;
    snprintf(e->message, sizeof(e->message), "%s",
             e->active ? "Road editor ON" : "Road editor OFF");
}

void road_editor_update_cursor(RoadEditor *e, MapPoint p, int valid) {
    e->mouse_map = p;
    e->mouse_valid = valid;
    e->snap_node_id = valid ? nearest_node(e, p) : -1;
}

void road_editor_add_node(RoadEditor *e) {
    EditorNode *n;
    if (!e->mouse_valid || e->node_count >= ROAD_EDITOR_MAX_NODES) return;
    if (e->snap_node_id >= 0) {
        snprintf(e->message, sizeof(e->message), "Node already exists: N%d",
                 e->nodes[e->snap_node_id].id);
        return;
    }
    n = &e->nodes[e->node_count];
    n->id = e->node_count + 1;
    snprintf(n->name, sizeof(n->name), "Node %d", n->id);
    n->map_x = (float)e->mouse_map.x;
    n->map_y = (float)e->mouse_map.y;
    n->type = NODE_JUNCTION;
    e->node_count++;
    e->snap_node_id = e->node_count - 1;
    snprintf(e->message, sizeof(e->message), "Created N%d", n->id);
}

void road_editor_add_control_point(RoadEditor *e) {
    MapPoint p;
    if (!e->mouse_valid || e->current_point_count >= MSP_MAX_GEOMETRY_POINTS) return;
    p = snapped_point(e);
    if (e->current_point_count == 0 && e->snap_node_id < 0) {
        snprintf(e->message, sizeof(e->message), "First point must snap to a Node");
        return;
    }
    if (e->current_point_count == 0) e->current_from_id = e->nodes[e->snap_node_id].id;
    e->current_points[e->current_point_count++] = p;
    snprintf(e->message, sizeof(e->message), "Geometry point %d", e->current_point_count);
}

void road_editor_undo(RoadEditor *e) {
    if (e->current_point_count > 0) {
        e->current_point_count--;
        if (e->current_point_count == 0) e->current_from_id = -1;
        snprintf(e->message, sizeof(e->message), "Last point undone");
    }
}

void road_editor_cancel(RoadEditor *e) {
    e->current_point_count = 0;
    e->current_from_id = -1;
    snprintf(e->message, sizeof(e->message), "Current edge cancelled");
}

int road_editor_finish_edge(RoadEditor *e) {
    EditorEdge *edge;
    int i, to_id;
    double length = 0.0;
    if (e->current_point_count < 1 || e->snap_node_id < 0) {
        snprintf(e->message, sizeof(e->message), "Edge needs 2+ points and a Node endpoint");
        return 0;
    }
    to_id = e->nodes[e->snap_node_id].id;
    if (to_id == e->current_from_id || e->edge_count >= ROAD_EDITOR_MAX_EDGES ||
        e->point_count + e->current_point_count > ROAD_EDITOR_MAX_POINTS) {
        snprintf(e->message, sizeof(e->message), "Invalid edge or editor capacity reached");
        return 0;
    }
    /* E/right-click confirms at the snapped node and preserves the last bend. */
    {
        MapPoint endpoint = snapped_point(e);
        MapPoint last = e->current_points[e->current_point_count - 1];
        if ((fabs(last.x - endpoint.x) > 1e-6 || fabs(last.y - endpoint.y) > 1e-6) &&
            e->current_point_count < MSP_MAX_GEOMETRY_POINTS) {
            e->current_points[e->current_point_count++] = endpoint;
        } else {
            e->current_points[e->current_point_count - 1] = endpoint;
        }
    }
    if (e->current_point_count < 2) {
        snprintf(e->message, sizeof(e->message), "Edge needs 2+ points");
        return 0;
    }
    edge = &e->edges[e->edge_count];
    edge->id = e->edge_count + 1;
    edge->from_id = e->current_from_id;
    edge->to_id = to_id;
    edge->type = ROAD_MAIN;
    edge->walkable = 1;
    edge->point_start = e->point_count;
    edge->point_count = e->current_point_count;
    for (i = 0; i < e->current_point_count; ++i) {
        e->points[e->point_count++] = e->current_points[i];
        if (i > 0) {
            double dx = e->current_points[i].x - e->current_points[i - 1].x;
            double dy = e->current_points[i].y - e->current_points[i - 1].y;
            length += sqrt(dx * dx + dy * dy);
        }
    }
    edge->weight = length;
    e->edge_count++;
    e->current_point_count = 0;
    e->current_from_id = -1;
    snprintf(e->message, sizeof(e->message), "Saved edge E%d", edge->id);
    return 1;
}

int road_editor_save(const RoadEditor *e, const char *dir) {
    char path[512];
    FILE *f;
    int i, j;
#ifdef _WIN32
    _mkdir(dir);
#else
    mkdir(dir, 0777);
#endif
    snprintf(path, sizeof(path), "%s/nodes.csv", dir);
    f = fopen(path, "w"); if (!f) return 0;
    fprintf(f, "id,name,map_x,map_y,type\n");
    for (i = 0; i < e->node_count; ++i) fprintf(f, "%d,%s,%.3f,%.3f,%d\n",
        e->nodes[i].id, e->nodes[i].name, e->nodes[i].map_x, e->nodes[i].map_y, e->nodes[i].type);
    fclose(f);
    snprintf(path, sizeof(path), "%s/edges.csv", dir);
    f = fopen(path, "w"); if (!f) return 0;
    fprintf(f, "edge_id,from_id,to_id,weight,type,walkable\n");
    for (i = 0; i < e->edge_count; ++i) fprintf(f, "%d,%d,%d,%.3f,%d,%d\n",
        e->edges[i].id, e->edges[i].from_id, e->edges[i].to_id, e->edges[i].weight,
        e->edges[i].type, e->edges[i].walkable);
    fclose(f);
    snprintf(path, sizeof(path), "%s/edge_geometry_points.csv", dir);
    f = fopen(path, "w"); if (!f) return 0;
    fprintf(f, "edge_id,point_index,map_x,map_y\n");
    for (i = 0; i < e->edge_count; ++i) for (j = 0; j < e->edges[i].point_count; ++j) {
        MapPoint p = e->points[e->edges[i].point_start + j];
        fprintf(f, "%d,%d,%.3f,%.3f\n", e->edges[i].id, j, p.x, p.y);
    }
    fclose(f);
    return 1;
}

static void line(MapPoint a, MapPoint b, const MapTransform *t, Color c, float width) {
    Vector3 wa = map_to_world((float)a.x, (float)a.y, t);
    Vector3 wb = map_to_world((float)b.x, (float)b.y, t);
    wa.y = wb.y = 0.10f;
    DrawLine3D(wa, wb, c);
    (void)width;
}

void road_editor_draw_world(const RoadEditor *e, const MapTransform *t) {
    int i, j;
    if (!e->active) return;
    for (i = 0; i < e->edge_count; ++i) for (j = 1; j < e->edges[i].point_count; ++j)
        line(e->points[e->edges[i].point_start+j-1], e->points[e->edges[i].point_start+j], t, BLUE, 2);
    if (e->debug_visible) {
        for (i = 0; i < e->point_count; ++i) {
            Vector3 p = map_to_world((float)e->points[i].x, (float)e->points[i].y, t);
            p.y = 0.115f;
            DrawSphere(p, 0.055f, SKYBLUE);
        }
    }
    for (i = 1; i < e->current_point_count; ++i)
        line(e->current_points[i-1], e->current_points[i], t, SKYBLUE, 2);
    if (e->mouse_valid && e->current_point_count > 0) {
        MapPoint cursor = snapped_point(e);
        line(e->current_points[e->current_point_count - 1], cursor, t,
             Fade(SKYBLUE, 0.72f), 2);
    }
    for (i = 0; i < e->current_point_count; ++i) {
        Vector3 p = map_to_world((float)e->current_points[i].x, (float)e->current_points[i].y, t);
        p.y = 0.12f; DrawSphere(p, 0.09f, i == 0 ? GREEN : (i+1 == e->current_point_count ? RED : WHITE));
    }
    for (i = 0; i < e->node_count; ++i) {
        Vector3 p = map_to_world(e->nodes[i].map_x, e->nodes[i].map_y, t);
        p.y = 0.13f; DrawSphere(p, i == e->snap_node_id ? 0.14f : 0.10f,
                               i == e->snap_node_id ? LIME : RED);
    }
}

void road_editor_draw_overlay(const RoadEditor *e, Font font) {
    char text[256];
    if (!e->active) return;
    DrawRectangle(310, 66, 430, 82, Fade(BLACK, 0.72f));
    snprintf(text, sizeof(text),
             "ROAD EDITOR | map: %.1f, %.1f | snap: %s | debug: %s",
             e->mouse_map.x, e->mouse_map.y, e->snap_node_id >= 0 ? "ON" : "OFF",
             e->debug_visible ? "ON" : "OFF");
    DrawTextEx(font, text, (Vector2){322, 76}, 18, 0, WHITE);
    DrawTextEx(font, "N node | Left point | E/Right finish | Z undo | Esc cancel | S save",
               (Vector2){322, 101}, 16, 0, RAYWHITE);
    DrawTextEx(font, e->message, (Vector2){322, 124}, 16, 0, YELLOW);
}
