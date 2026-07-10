/* AI-assisted modification: added map persistence support. */
#include "storage.h"

#include "graph.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        place.entrance_node_id = atoi(fields[3]);
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
