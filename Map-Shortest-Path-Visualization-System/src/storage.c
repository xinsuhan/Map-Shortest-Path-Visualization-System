/* AI-assisted modification: added map persistence support. */
#include "storage.h"

#include "graph.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char external_id[MSP_EXTERNAL_ID_LENGTH];
    int node_id;
} ExternalNodeRef;

typedef struct {
    char external_id[MSP_EXTERNAL_ID_LENGTH];
    int edge_index;
    int owns_geometry;
    int expected_geometry_count;
} ExternalEdgeRef;

static void set_node_defaults(Node *node) {
    node->type = NODE_LANDMARK;
    node->width = 1.0f;
    node->depth = 0.8f;
    node->height = 0.5f;
    node->visible = 1;
}

static int default_node_visible(NodeType type) {
    return type == NODE_JUNCTION || type == NODE_BRIDGE ||
           type == NODE_ENTRANCE || type == NODE_SERVICE
               ? 0
               : 1;
}

const char *storage_node_type_name(NodeType type) {
    switch (type) {
        case NODE_JUNCTION: return "JUNCTION";
        case NODE_LAKE: return "LAKE";
        case NODE_SQUARE: return "SQUARE";
        case NODE_GATE: return "GATE";
        case NODE_BRIDGE: return "BRIDGE";
        case NODE_ENTRANCE: return "ENTRANCE";
        case NODE_SERVICE: return "SERVICE";
        case NODE_LANDMARK:
        default: return "LANDMARK";
    }
}

const char *storage_road_type_name(RoadType type) {
    switch (type) {
        case ROAD_BRANCH: return "ROAD_BRANCH";
        case ROAD_ENTRANCE: return "ROAD_ENTRANCE";
        case ROAD_BRIDGE: return "ROAD_BRIDGE";
        case ROAD_MAIN:
        default: return "ROAD_MAIN";
    }
}

static int parse_node_type(const char *text, NodeType *type) {
    if (strcmp(text, "LANDMARK") == 0) *type = NODE_LANDMARK;
    else if (strcmp(text, "JUNCTION") == 0) *type = NODE_JUNCTION;
    else if (strcmp(text, "LAKE") == 0) *type = NODE_LAKE;
    else if (strcmp(text, "SQUARE") == 0) *type = NODE_SQUARE;
    else if (strcmp(text, "GATE") == 0) *type = NODE_GATE;
    else if (strcmp(text, "BRIDGE") == 0) *type = NODE_BRIDGE;
    else if (strcmp(text, "ENTRANCE") == 0) *type = NODE_ENTRANCE;
    else if (strcmp(text, "SERVICE") == 0) *type = NODE_SERVICE;
    else return 0;
    return 1;
}

static int parse_road_type(const char *text, RoadType *type) {
    if (strcmp(text, "ROAD_MAIN") == 0 || strcmp(text, "MAIN") == 0) *type = ROAD_MAIN;
    else if (strcmp(text, "ROAD_BRANCH") == 0 || strcmp(text, "BRANCH") == 0) *type = ROAD_BRANCH;
    else if (strcmp(text, "ROAD_ENTRANCE") == 0 || strcmp(text, "ENTRANCE") == 0) *type = ROAD_ENTRANCE;
    else if (strcmp(text, "ROAD_BRIDGE") == 0 || strcmp(text, "BRIDGE") == 0) *type = ROAD_BRIDGE;
    else return 0;
    return 1;
}

static void trim_newline(char *text) {
    size_t length = strlen(text);
    while (length > 0 && (text[length - 1] == '\n' || text[length - 1] == '\r')) {
        text[--length] = '\0';
    }
    if (length >= 3 && (unsigned char)text[0] == 0xEF &&
        (unsigned char)text[1] == 0xBB && (unsigned char)text[2] == 0xBF) {
        memmove(text, text + 3, length - 2);
    }
}

static int split_csv_line(char *line, char *fields[], int max_fields) {
    int count = 0;
    char *token = strtok(line, ",");
    while (token != NULL && count < max_fields) {
        fields[count++] = token;
        token = strtok(NULL, ",");
    }
    return count;
}

static int split_whitespace_line(char *line, char *fields[], int max_fields) {
    int count = 0;
    char *token = strtok(line, " \t");
    while (token != NULL && count < max_fields) {
        fields[count++] = token;
        token = strtok(NULL, " \t");
    }
    return count;
}

static int parse_double_strict(const char *text, double *value) {
    char *end;
    if (text == NULL || value == NULL || text[0] == '\0') {
        return 0;
    }
    *value = strtod(text, &end);
    return end != text && *end == '\0';
}

static int parse_int_strict(const char *text, int *value) {
    char *end;
    long parsed;
    if (text == NULL || value == NULL || text[0] == '\0') {
        return 0;
    }
    parsed = strtol(text, &end, 10);
    if (end == text || *end != '\0') {
        return 0;
    }
    *value = (int)parsed;
    return 1;
}

static int external_node_id(const ExternalNodeRef refs[], int count,
                            const char *external_id) {
    int i;
    for (i = 0; i < count; ++i) {
        if (strcmp(refs[i].external_id, external_id) == 0) {
            return refs[i].node_id;
        }
    }
    return -1;
}

static int external_edge_ref_index(const ExternalEdgeRef refs[], int count,
                                   const char *external_id) {
    int i;
    for (i = 0; i < count; ++i) {
        if (strcmp(refs[i].external_id, external_id) == 0) {
            return i;
        }
    }
    return -1;
}

static int existing_undirected_edge(const Graph *graph, int from_id, int to_id) {
    int i;
    for (i = 0; i < graph->edge_count; ++i) {
        const Edge *edge = &graph->edges[i];
        if ((edge->from_id == from_id && edge->to_id == to_id) ||
            (edge->from_id == to_id && edge->to_id == from_id)) {
            return i;
        }
    }
    return -1;
}

static int load_curved_nodes(const char *file_path, Graph *graph,
                             ExternalNodeRef refs[], int *ref_count) {
    FILE *file;
    char line[MSP_LINE_LENGTH];
    int line_number = 0;
    if (file_path == NULL || graph == NULL || refs == NULL || ref_count == NULL) {
        return MSP_ERROR_INVALID_ARGUMENT;
    }
    file = fopen(file_path, "r");
    if (file == NULL) {
        return MSP_ERROR_IO;
    }
    *ref_count = 0;
    while (fgets(line, sizeof(line), file) != NULL) {
        char *fields[7];
        int field_count;
        Node node;
        double x;
        double y;
        line_number++;
        trim_newline(line);
        if (line[0] == '\0' || line[0] == '#') continue;
        field_count = split_csv_line(line, fields, 7);
        if (line_number == 1 && field_count == 7 && strcmp(fields[0], "id") == 0) {
            continue;
        }
        if (field_count != 7 || *ref_count >= MSP_MAX_NODES ||
            external_node_id(refs, *ref_count, fields[0]) >= 0 ||
            !parse_double_strict(fields[1], &x) ||
            !parse_double_strict(fields[2], &y) ||
            strcmp(fields[6], "intersection") != 0) {
            fclose(file);
            return MSP_ERROR_FORMAT;
        }
        memset(&node, 0, sizeof(node));
        set_node_defaults(&node);
        node.id = *ref_count;
        snprintf(node.name, sizeof(node.name), "%s", fields[0]);
        node.x = x;
        node.y = y;
        node.type = NODE_JUNCTION;
        node.visible = 0;
        node.width = 16.0f;
        node.depth = 16.0f;
        node.height = 0.05f;
        if (graph_add_node(graph, node) != MSP_OK) {
            fclose(file);
            return MSP_ERROR_FORMAT;
        }
        snprintf(refs[*ref_count].external_id,
                 sizeof(refs[*ref_count].external_id), "%s", fields[0]);
        refs[*ref_count].node_id = node.id;
        (*ref_count)++;
    }
    fclose(file);
    return *ref_count > 0 ? MSP_OK : MSP_ERROR_FORMAT;
}

static RoadType curved_road_type(const char *kind, const char *oneway,
                                 const char *width) {
    const char *effective_width = strcmp(oneway, "wide") == 0 ? "wide" : width;
    if (strcmp(kind, "bridge") == 0) return ROAD_BRIDGE;
    if (strcmp(kind, "pedestrian") == 0) return ROAD_ENTRANCE;
    return strcmp(effective_width, "wide") == 0 ? ROAD_MAIN : ROAD_BRANCH;
}

static int curved_edge_is_bidirectional(const char *oneway, int *bidirectional) {
    if (strcmp(oneway, "False") == 0 || strcmp(oneway, "false") == 0 ||
        strcmp(oneway, "0") == 0 || strcmp(oneway, "wide") == 0) {
        *bidirectional = 1;
        return 1;
    }
    if (strcmp(oneway, "True") == 0 || strcmp(oneway, "true") == 0 ||
        strcmp(oneway, "1") == 0) {
        *bidirectional = 0;
        return 1;
    }
    return 0;
}

static int load_curved_edges(const char *file_path, Graph *graph,
                             const ExternalNodeRef node_refs[], int node_ref_count,
                             ExternalEdgeRef edge_refs[], int *edge_ref_count) {
    FILE *file;
    char line[MSP_LINE_LENGTH];
    int line_number = 0;
    if (file_path == NULL || graph == NULL || node_refs == NULL || edge_refs == NULL ||
        edge_ref_count == NULL) {
        return MSP_ERROR_INVALID_ARGUMENT;
    }
    file = fopen(file_path, "r");
    if (file == NULL) return MSP_ERROR_IO;
    *edge_ref_count = 0;
    while (fgets(line, sizeof(line), file) != NULL) {
        char *fields[13];
        int field_count;
        int from_id;
        int to_id;
        int bidirectional;
        int expected_points;
        int walkable;
        int edge_index;
        int status;
        double curved_weight;
        RoadType type;
        line_number++;
        trim_newline(line);
        if (line[0] == '\0' || line[0] == '#') continue;
        field_count = split_csv_line(line, fields, 13);
        if (line_number == 1 && field_count == 13 && strcmp(fields[0], "id") == 0) {
            continue;
        }
        if (field_count != 13 || *edge_ref_count >= MSP_MAX_EDGES ||
            external_edge_ref_index(edge_refs, *edge_ref_count, fields[0]) >= 0 ||
            !parse_double_strict(fields[10], &curved_weight) || curved_weight <= 0.0 ||
            !parse_int_strict(fields[11], &expected_points) || expected_points < 2 ||
            !parse_int_strict(fields[12], &walkable) ||
            (walkable != 0 && walkable != 1) ||
            !curved_edge_is_bidirectional(fields[5], &bidirectional)) {
            fclose(file);
            return MSP_ERROR_FORMAT;
        }
        from_id = external_node_id(node_refs, node_ref_count, fields[1]);
        to_id = external_node_id(node_refs, node_ref_count, fields[2]);
        if (from_id < 0 || to_id < 0) {
            fclose(file);
            return MSP_ERROR_FORMAT;
        }
        type = curved_road_type(fields[4], fields[5], fields[6]);
        edge_index = existing_undirected_edge(graph, from_id, to_id);
        edge_refs[*edge_ref_count].owns_geometry = edge_index < 0;
        if (edge_index < 0) {
            status = graph_add_road_edge(graph, from_id, to_id, curved_weight,
                                         type, walkable, bidirectional);
            if (status != MSP_OK) {
                fclose(file);
                return status;
            }
            edge_index = graph->edge_count - 1;
        } else {
            Edge *existing = &graph->edges[edge_index];
            if (fabs(existing->weight - curved_weight) > 0.01) {
                fclose(file);
                return MSP_ERROR_FORMAT;
            }
            if (existing->walkable != walkable) {
                fclose(file);
                return MSP_ERROR_FORMAT;
            }
            if (type == ROAD_BRIDGE) existing->type = ROAD_BRIDGE;
        }
        snprintf(edge_refs[*edge_ref_count].external_id,
                 sizeof(edge_refs[*edge_ref_count].external_id), "%s", fields[0]);
        edge_refs[*edge_ref_count].edge_index = edge_index;
        edge_refs[*edge_ref_count].expected_geometry_count = expected_points;
        (*edge_ref_count)++;
    }
    fclose(file);
    return *edge_ref_count > 0 ? MSP_OK : MSP_ERROR_FORMAT;
}

static int geometry_matches_node(const MapPoint *point, const Node *node) {
    return point != NULL && node != NULL && fabs(point->x - node->x) < 0.001 &&
           fabs(point->y - node->y) < 0.001;
}

static int load_curved_geometry(const char *file_path, Graph *graph,
                                const ExternalEdgeRef edge_refs[], int edge_ref_count) {
    FILE *file;
    char line[MSP_LINE_LENGTH];
    int line_number = 0;
    int i;
    if (file_path == NULL || graph == NULL || edge_refs == NULL) {
        return MSP_ERROR_INVALID_ARGUMENT;
    }
    file = fopen(file_path, "r");
    if (file == NULL) return MSP_ERROR_IO;
    while (fgets(line, sizeof(line), file) != NULL) {
        char *fields[7];
        int field_count;
        int ref_index;
        int point_order;
        double x;
        double y;
        line_number++;
        trim_newline(line);
        if (line[0] == '\0' || line[0] == '#') continue;
        field_count = split_csv_line(line, fields, 7);
        if (line_number == 1 && field_count == 7 && strcmp(fields[0], "edge_id") == 0) {
            continue;
        }
        ref_index = field_count == 7
                        ? external_edge_ref_index(edge_refs, edge_ref_count, fields[0])
                        : -1;
        if (field_count != 7 || ref_index < 0 ||
            !parse_int_strict(fields[4], &point_order) ||
            !parse_double_strict(fields[5], &x) ||
            !parse_double_strict(fields[6], &y)) {
            fclose(file);
            return MSP_ERROR_FORMAT;
        }
        if (!edge_refs[ref_index].owns_geometry) continue;
        if (graph->edges[edge_refs[ref_index].edge_index].geometry_count != point_order ||
            graph_append_edge_geometry_point(graph, edge_refs[ref_index].edge_index,
                                             x, y) != MSP_OK) {
            fclose(file);
            return MSP_ERROR_FORMAT;
        }
    }
    fclose(file);
    for (i = 0; i < edge_ref_count; ++i) {
        const ExternalEdgeRef *ref = &edge_refs[i];
        const Edge *edge;
        const Node *from;
        const Node *to;
        const MapPoint *first;
        const MapPoint *last;
        if (!ref->owns_geometry) continue;
        edge = &graph->edges[ref->edge_index];
        if (edge->geometry_count != ref->expected_geometry_count ||
            edge->geometry_start < 0) return MSP_ERROR_FORMAT;
        from = graph_get_node(graph, edge->from_id);
        to = graph_get_node(graph, edge->to_id);
        first = &graph->geometry_points[edge->geometry_start];
        last = &graph->geometry_points[edge->geometry_start + edge->geometry_count - 1];
        if (!geometry_matches_node(first, from) || !geometry_matches_node(last, to)) {
            return MSP_ERROR_FORMAT;
        }
    }
    return graph->geometry_point_count > 0 ? MSP_OK : MSP_ERROR_FORMAT;
}

static NodeType curved_poi_node_type(const char *category) {
    if (strcmp(category, "lake") == 0) return NODE_LAKE;
    if (strcmp(category, "sports") == 0) return NODE_SQUARE;
    return NODE_LANDMARK;
}

static void set_curved_poi_dimensions(Node *node, const char *category) {
    node->width = 72.0f;
    node->depth = 48.0f;
    node->height = 0.58f;
    if (strcmp(category, "lake") == 0) {
        node->width = 170.0f;
        node->depth = 115.0f;
        node->height = 0.05f;
    } else if (strcmp(category, "sports") == 0) {
        node->width = 145.0f;
        node->depth = 88.0f;
        node->height = 0.05f;
    } else if (strcmp(category, "dormitory") == 0) {
        node->width = 92.0f;
        node->depth = 52.0f;
        node->height = 0.52f;
    }
}

static int load_curved_pois(const char *file_path, Graph *graph,
                            const ExternalNodeRef node_refs[], int node_ref_count,
                            PlaceStore *places) {
    FILE *file;
    char line[MSP_LINE_LENGTH];
    int line_number = 0;
    if (file_path == NULL || graph == NULL || node_refs == NULL || places == NULL) {
        return MSP_ERROR_INVALID_ARGUMENT;
    }
    file = fopen(file_path, "r");
    if (file == NULL) return MSP_ERROR_IO;
    places->place_count = 0;
    while (fgets(line, sizeof(line), file) != NULL) {
        char *fields[6];
        int field_count;
        int nearest_id;
        double x;
        double y;
        Node node;
        Place place;
        const Node *nearest;
        int anchor_edge_index;
        double anchor_weight;
        line_number++;
        trim_newline(line);
        if (line[0] == '\0' || line[0] == '#') continue;
        field_count = split_csv_line(line, fields, 6);
        if (line_number == 1 && field_count == 6 && strcmp(fields[0], "id") == 0) {
            continue;
        }
        if (field_count != 6 || graph->node_count >= MSP_MAX_NODES ||
            places->place_count >= MSP_MAX_PLACES ||
            !parse_double_strict(fields[2], &x) ||
            !parse_double_strict(fields[3], &y)) {
            fclose(file);
            return MSP_ERROR_FORMAT;
        }
        nearest_id = external_node_id(node_refs, node_ref_count, fields[5]);
        nearest = graph_get_node(graph, nearest_id);
        if (nearest == NULL) {
            fclose(file);
            return MSP_ERROR_FORMAT;
        }
        memset(&node, 0, sizeof(node));
        set_node_defaults(&node);
        node.id = graph->node_count;
        snprintf(node.name, sizeof(node.name), "%s", fields[1]);
        node.x = x;
        node.y = y;
        node.type = curved_poi_node_type(fields[4]);
        node.visible = 1;
        set_curved_poi_dimensions(&node, fields[4]);
        if (graph_add_node(graph, node) != MSP_OK) {
            fclose(file);
            return MSP_ERROR_FORMAT;
        }
        /* Keep a non-walkable display anchor in the edge/geometry model so the
         * POI's visual association remains explicit without making routing or
         * road rendering draw a straight segment through the POI footprint. */
        anchor_edge_index = graph->edge_count;
        anchor_weight = hypot(node.x - nearest->x, node.y - nearest->y) * 0.8;
        if (anchor_weight < MSP_EPSILON) anchor_weight = MSP_EPSILON;
        if (graph_add_road_edge(graph, node.id, nearest_id, anchor_weight,
                                ROAD_ENTRANCE, 0, 1) != MSP_OK ||
            graph_append_edge_geometry_point(graph, anchor_edge_index,
                                             node.x, node.y) != MSP_OK ||
            graph_append_edge_geometry_point(graph, anchor_edge_index,
                                             nearest->x, nearest->y) != MSP_OK) {
            fclose(file);
            return MSP_ERROR_FORMAT;
        }
        memset(&place, 0, sizeof(place));
        place.id = places->place_count;
        snprintf(place.name, sizeof(place.name), "%s", node.name);
        snprintf(place.alias, sizeof(place.alias), "%s", fields[1]);
        place.display_node_id = node.id;
        place.entrance_node_id = nearest_id;
        snprintf(place.category, sizeof(place.category), "%s", fields[4]);
        places->places[places->place_count++] = place;
    }
    fclose(file);
    return places->place_count > 0 ? MSP_OK : MSP_ERROR_FORMAT;
}

int storage_load_nodes(const char *file_path, Graph *graph) {
    FILE *file;
    char line[MSP_LINE_LENGTH];
    int line_number = 0;

    if (file_path == NULL || graph == NULL) {
        return MSP_ERROR_INVALID_ARGUMENT;
    }
    file = fopen(file_path, "r");
    if (file == NULL) {
        return MSP_ERROR_IO;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        Node node;
        char *fields[9];
        int field_count;
        line_number++;
        trim_newline(line);
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        set_node_defaults(&node);
        field_count = split_csv_line(line, fields, 9);
        if (field_count != 4 && field_count != 6 && field_count != 8 && field_count != 9) {
            if (line_number == 1) {
                continue;
            }
            fclose(file);
            return MSP_ERROR_FORMAT;
        }
        if (line_number == 1 && strcmp(fields[0], "id") == 0) {
            continue;
        }
        node.id = atoi(fields[0]);
        snprintf(node.name, sizeof(node.name), "%s", fields[1]);
        node.x = strtod(fields[2], NULL);
        node.y = strtod(fields[3], NULL);
        if (field_count >= 6) {
            if (!parse_node_type(fields[4], &node.type)) {
                fclose(file);
                return MSP_ERROR_FORMAT;
            }
            node.visible = default_node_visible(node.type);
            if (field_count == 6) {
                node.visible = atoi(fields[5]) ? 1 : 0;
            } else if (field_count == 8) {
                node.width = (float)strtod(fields[5], NULL);
                node.depth = (float)strtod(fields[6], NULL);
                node.height = (float)strtod(fields[7], NULL);
            } else {
                node.visible = atoi(fields[5]) ? 1 : 0;
                node.width = (float)strtod(fields[6], NULL);
                node.depth = (float)strtod(fields[7], NULL);
                node.height = (float)strtod(fields[8], NULL);
            }
            if (node.width <= 0.0f || node.depth <= 0.0f || node.height <= 0.0f) {
                fclose(file);
                return MSP_ERROR_FORMAT;
            }
        }
        if (graph_add_node(graph, node) != MSP_OK) {
            fclose(file);
            return MSP_ERROR_FORMAT;
        }
    }
    fclose(file);
    return graph->node_count > 0 ? MSP_OK : MSP_ERROR_FORMAT;
}

int storage_load_edges(const char *file_path, Graph *graph, int bidirectional) {
    FILE *file;
    char line[MSP_LINE_LENGTH];
    int line_number = 0;

    if (file_path == NULL || graph == NULL) {
        return MSP_ERROR_INVALID_ARGUMENT;
    }
    file = fopen(file_path, "r");
    if (file == NULL) {
        return MSP_ERROR_IO;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        int from_id;
        int to_id;
        double weight;
        RoadType type = ROAD_MAIN;
        int walkable = 1;
        char *fields[5];
        int field_count;
        line_number++;
        trim_newline(line);
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        field_count = split_csv_line(line, fields, 5);
        if (field_count != 3 && field_count != 5) {
            if (line_number == 1) {
                continue;
            }
            fclose(file);
            return MSP_ERROR_FORMAT;
        }
        if (line_number == 1 && (strcmp(fields[0], "from_id") == 0 ||
                                 strcmp(fields[0], "from") == 0)) {
            continue;
        }
        from_id = atoi(fields[0]);
        to_id = atoi(fields[1]);
        weight = strtod(fields[2], NULL);
        if (field_count == 5) {
            if (!parse_road_type(fields[3], &type) ||
                (strcmp(fields[4], "0") != 0 && strcmp(fields[4], "1") != 0)) {
                fclose(file);
                return MSP_ERROR_FORMAT;
            }
            walkable = fields[4][0] == '1';
        }
        if (graph_add_road_edge(graph, from_id, to_id, weight, type, walkable,
                                bidirectional) != MSP_OK) {
            fclose(file);
            return MSP_ERROR_FORMAT;
        }
    }
    fclose(file);
    return graph->edge_count > 0 ? MSP_OK : MSP_ERROR_FORMAT;
}

int storage_load_graph(const char *nodes_path, const char *edges_path, Graph *graph) {
    int status;
    if (graph == NULL) {
        return MSP_ERROR_INVALID_ARGUMENT;
    }
    graph_init(graph);
    status = storage_load_nodes(nodes_path, graph);
    if (status != MSP_OK) {
        return status;
    }
    return storage_load_edges(edges_path, graph, 1);
}

int storage_load_curved_campus(const char *nodes_path, const char *edges_path,
                               const char *geometry_path, const char *pois_path,
                               Graph *graph, PlaceStore *places) {
    ExternalNodeRef node_refs[MSP_MAX_NODES];
    ExternalEdgeRef edge_refs[MSP_MAX_EDGES];
    int node_ref_count = 0;
    int edge_ref_count = 0;
    int status;
    if (nodes_path == NULL || edges_path == NULL || geometry_path == NULL ||
        pois_path == NULL || graph == NULL || places == NULL) {
        return MSP_ERROR_INVALID_ARGUMENT;
    }
    graph_init(graph);
    places->place_count = 0;
    status = load_curved_nodes(nodes_path, graph, node_refs, &node_ref_count);
    if (status == MSP_OK) {
        status = load_curved_edges(edges_path, graph, node_refs, node_ref_count,
                                   edge_refs, &edge_ref_count);
    }
    if (status == MSP_OK) {
        status = load_curved_geometry(geometry_path, graph, edge_refs, edge_ref_count);
    }
    if (status == MSP_OK) {
        status = load_curved_pois(pois_path, graph, node_refs, node_ref_count, places);
    }
    if (status != MSP_OK) {
        graph_init(graph);
        places->place_count = 0;
        return status;
    }
    graph->weights_in_meters = 1;
    return MSP_OK;
}

int storage_load_places(const char *file_path, const Graph *graph, PlaceStore *places) {
    FILE *file;
    char line[MSP_LINE_LENGTH];
    int line_number = 0;

    if (file_path == NULL || graph == NULL || places == NULL) {
        return MSP_ERROR_INVALID_ARGUMENT;
    }
    places->place_count = 0;
    file = fopen(file_path, "r");
    if (file == NULL) {
        return MSP_ERROR_IO;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        char *fields[5];
        int field_count;
        Place place;

        line_number++;
        trim_newline(line);
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        field_count = split_csv_line(line, fields, 5);
        if (field_count != 5) {
            if (line_number == 1) {
                continue;
            }
            fclose(file);
            return MSP_ERROR_FORMAT;
        }
        if (line_number == 1 && strcmp(fields[0], "id") == 0) {
            continue;
        }
        if (places->place_count >= MSP_MAX_PLACES) {
            fclose(file);
            return MSP_ERROR_CAPACITY;
        }

        place.id = atoi(fields[0]);
        snprintf(place.name, sizeof(place.name), "%s", fields[1]);
        snprintf(place.alias, sizeof(place.alias), "%s", fields[2]);
        place.display_node_id = atoi(fields[3]);
        place.entrance_node_id = place.display_node_id;
        snprintf(place.category, sizeof(place.category), "%s", fields[4]);
        {
            const Node *entrance = graph_get_node(graph, place.entrance_node_id);
            int duplicate = 0;
            int i;
            for (i = 0; i < places->place_count; ++i) {
                if (places->places[i].id == place.id) {
                    duplicate = 1;
                    break;
                }
            }
            if (place.name[0] == '\0' || duplicate || entrance == NULL ||
                (entrance->type != NODE_GATE && entrance->type != NODE_ENTRANCE &&
                 entrance->type != NODE_SERVICE)) {
                fclose(file);
                places->place_count = 0;
                return MSP_ERROR_FORMAT;
            }
        }
        places->places[places->place_count++] = place;
    }
    fclose(file);
    return places->place_count > 0 ? MSP_OK : MSP_ERROR_FORMAT;
}

const Place *storage_find_place(const PlaceStore *places, int place_id) {
    int i;
    if (places == NULL) {
        return NULL;
    }
    for (i = 0; i < places->place_count; ++i) {
        if (places->places[i].id == place_id) {
            return &places->places[i];
        }
    }
    return NULL;
}

const Place *storage_find_place_by_display(const PlaceStore *places, int display_node_id) {
    int i;
    if (places == NULL) {
        return NULL;
    }
    for (i = 0; i < places->place_count; ++i) {
        if (places->places[i].display_node_id == display_node_id) {
            return &places->places[i];
        }
    }
    return NULL;
}

const Place *storage_find_place_by_entrance(const PlaceStore *places, int entrance_node_id) {
    int i;
    if (places == NULL) {
        return NULL;
    }
    for (i = 0; i < places->place_count; ++i) {
        if (places->places[i].entrance_node_id == entrance_node_id) {
            return &places->places[i];
        }
    }
    return NULL;
}

int storage_load_map(const char *file_path, Graph *graph) {
    FILE *file;
    char line[MSP_LINE_LENGTH];
    char section[16];
    int node_count;
    int edge_count;
    int i;

    if (file_path == NULL || graph == NULL) {
        return MSP_ERROR_INVALID_ARGUMENT;
    }
    file = fopen(file_path, "r");
    if (file == NULL) {
        return MSP_ERROR_IO;
    }

    graph_init(graph);
    if (fscanf(file, "%15s %d", section, &node_count) != 2 ||
        strcmp(section, "NODES") != 0 || node_count < 1 || node_count > MSP_MAX_NODES) {
        fclose(file);
        return MSP_ERROR_FORMAT;
    }
    fgets(line, sizeof(line), file);
    for (i = 0; i < node_count; ++i) {
        Node node;
        char *fields[9];
        int field_count;
        if (fgets(line, sizeof(line), file) == NULL) {
            fclose(file);
            return MSP_ERROR_FORMAT;
        }
        trim_newline(line);
        set_node_defaults(&node);
        field_count = split_whitespace_line(line, fields, 9);
        if (field_count != 4 && field_count != 6 && field_count != 8 &&
            field_count != 9) {
            fclose(file);
            return MSP_ERROR_FORMAT;
        }
        node.id = atoi(fields[0]);
        snprintf(node.name, sizeof(node.name), "%s", fields[1]);
        node.x = strtod(fields[2], NULL);
        node.y = strtod(fields[3], NULL);
        if (field_count >= 6) {
            if (!parse_node_type(fields[4], &node.type)) {
                fclose(file);
                return MSP_ERROR_FORMAT;
            }
            node.visible = default_node_visible(node.type);
            if (field_count == 6) {
                node.visible = atoi(fields[5]) ? 1 : 0;
            } else if (field_count == 8) {
                node.width = (float)strtod(fields[5], NULL);
                node.depth = (float)strtod(fields[6], NULL);
                node.height = (float)strtod(fields[7], NULL);
            } else {
                node.visible = atoi(fields[5]) ? 1 : 0;
                node.width = (float)strtod(fields[6], NULL);
                node.depth = (float)strtod(fields[7], NULL);
                node.height = (float)strtod(fields[8], NULL);
            }
            if (node.width <= 0.0f || node.depth <= 0.0f || node.height <= 0.0f) {
                fclose(file);
                return MSP_ERROR_FORMAT;
            }
        }
        if (graph_add_node(graph, node) != MSP_OK) {
            fclose(file);
            return MSP_ERROR_FORMAT;
        }
    }

    if (fscanf(file, "%15s %d", section, &edge_count) != 2 ||
        strcmp(section, "EDGES") != 0 || edge_count < 1 || edge_count > MSP_MAX_EDGES) {
        fclose(file);
        return MSP_ERROR_FORMAT;
    }
    fgets(line, sizeof(line), file);
    for (i = 0; i < edge_count; ++i) {
        int from_id;
        int to_id;
        double weight;
        RoadType type = ROAD_MAIN;
        int walkable = 1;
        int parsed;
        if (fgets(line, sizeof(line), file) == NULL) {
            fclose(file);
            return MSP_ERROR_FORMAT;
        }
        parsed = sscanf(line, "%d %d %lf %15s %d", &from_id, &to_id, &weight,
                        section, &walkable);
        if ((parsed != 3 && parsed != 5) || weight <= 0.0 ||
            (parsed == 5 && !parse_road_type(section, &type)) ||
            graph_add_road_edge(graph, from_id, to_id, weight, type, walkable, 1) != MSP_OK) {
            fclose(file);
            return MSP_ERROR_FORMAT;
        }
    }

    fclose(file);
    return MSP_OK;
}

int storage_save_map(const char *file_path, const Graph *graph) {
    FILE *file;
    int i;

    if (file_path == NULL || graph == NULL) {
        return MSP_ERROR_INVALID_ARGUMENT;
    }
    file = fopen(file_path, "w");
    if (file == NULL) {
        return MSP_ERROR_IO;
    }

    if (fprintf(file, "NODES %d\n", graph->node_count) < 0) {
        fclose(file);
        return MSP_ERROR_IO;
    }
    for (i = 0; i < graph->node_count; ++i) {
        const Node *node = &graph->nodes[i];
        int written;
        if (node->width > 0.0f && node->depth > 0.0f && node->height > 0.0f) {
            written = fprintf(file, "%d %s %.17g %.17g %s %d %.6g %.6g %.6g\n",
                              node->id, node->name, node->x, node->y,
                              storage_node_type_name(node->type), node->visible,
                              node->width, node->depth, node->height);
        } else {
            written = fprintf(file, "%d %s %.17g %.17g\n", node->id,
                              node->name, node->x, node->y);
        }
        if (written < 0) {
            fclose(file);
            return MSP_ERROR_IO;
        }
    }

    if (fprintf(file, "EDGES %d\n", graph->edge_count) < 0) {
        fclose(file);
        return MSP_ERROR_IO;
    }
    for (i = 0; i < graph->edge_count; ++i) {
        const Edge *edge = &graph->edges[i];
        if (fprintf(file, "%d %d %.17g %s %d\n", edge->from_id, edge->to_id,
                    edge->weight, storage_road_type_name(edge->type),
                    edge->walkable) < 0) {
            fclose(file);
            return MSP_ERROR_IO;
        }
    }

    return fclose(file) == 0 ? MSP_OK : MSP_ERROR_IO;
}

int storage_save_path(const char *file_path, const Graph *graph, const PathResult *result) {
    FILE *file;
    int i;

    if (file_path == NULL || graph == NULL || result == NULL || !result->found) {
        return MSP_ERROR_INVALID_ARGUMENT;
    }
    file = fopen(file_path, "w");
    if (file == NULL) {
        return MSP_ERROR_IO;
    }
    fprintf(file, "sequence,node_id,node_name\n");
    for (i = 0; i < result->path_length; ++i) {
        const Node *node = graph_get_node(graph, result->path[i]);
        fprintf(file, "%d,%d,%s\n", i + 1, result->path[i], node != NULL ? node->name : "unknown");
    }
    fprintf(file, "total_distance,,%.2f\n", result->total_distance);
    fclose(file);
    return MSP_OK;
}
