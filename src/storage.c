/* AI-assisted modification: added map persistence support. */
#include "storage.h"

#include "graph.h"

#include <stdio.h>
#include <string.h>

static void set_node_defaults(Node *node) {
    node->type = NODE_LANDMARK;
    node->width = 1.0f;
    node->depth = 0.8f;
    node->height = 0.5f;
}

const char *storage_node_type_name(NodeType type) {
    switch (type) {
        case NODE_JUNCTION: return "JUNCTION";
        case NODE_LAKE: return "LAKE";
        case NODE_SQUARE: return "SQUARE";
        case NODE_GATE: return "GATE";
        case NODE_LANDMARK:
        default: return "LANDMARK";
    }
}

static int parse_node_type(const char *text, NodeType *type) {
    if (strcmp(text, "LANDMARK") == 0) *type = NODE_LANDMARK;
    else if (strcmp(text, "JUNCTION") == 0) *type = NODE_JUNCTION;
    else if (strcmp(text, "LAKE") == 0) *type = NODE_LAKE;
    else if (strcmp(text, "SQUARE") == 0) *type = NODE_SQUARE;
    else if (strcmp(text, "GATE") == 0) *type = NODE_GATE;
    else return 0;
    return 1;
}

static void trim_newline(char *text) {
    size_t length = strlen(text);
    while (length > 0 && (text[length - 1] == '\n' || text[length - 1] == '\r')) {
        text[--length] = '\0';
    }
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
        char type_name[16];
        int parsed;
        line_number++;
        trim_newline(line);
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        set_node_defaults(&node);
        parsed = sscanf(line, "%d,%63[^,],%lf,%lf,%15[^,],%f,%f,%f", &node.id,
                        node.name, &node.x, &node.y, type_name, &node.width,
                        &node.depth, &node.height);
        if (parsed != 4 && parsed != 8) {
            if (line_number == 1) {
                continue;
            }
            fclose(file);
            return MSP_ERROR_FORMAT;
        }
        if (parsed == 8 && (!parse_node_type(type_name, &node.type) ||
                            node.width <= 0.0f || node.depth <= 0.0f ||
                            node.height <= 0.0f)) {
            fclose(file);
            return MSP_ERROR_FORMAT;
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
        int parsed;
        line_number++;
        trim_newline(line);
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        parsed = sscanf(line, "%d,%d,%lf", &from_id, &to_id, &weight);
        if (parsed != 3) {
            if (line_number == 1) {
                continue;
            }
            fclose(file);
            return MSP_ERROR_FORMAT;
        }
        if (graph_add_edge(graph, from_id, to_id, weight, bidirectional) != MSP_OK) {
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
        char type_name[16];
        int parsed;
        if (fgets(line, sizeof(line), file) == NULL) {
            fclose(file);
            return MSP_ERROR_FORMAT;
        }
        set_node_defaults(&node);
        parsed = sscanf(line, "%d %63s %lf %lf %15s %f %f %f", &node.id,
                        node.name, &node.x, &node.y, type_name, &node.width,
                        &node.depth, &node.height);
        if ((parsed != 4 && parsed != 8) ||
            (parsed == 8 && (!parse_node_type(type_name, &node.type) ||
                             node.width <= 0.0f || node.depth <= 0.0f ||
                             node.height <= 0.0f)) ||
            graph_add_node(graph, node) != MSP_OK) {
            fclose(file);
            return MSP_ERROR_FORMAT;
        }
    }

    if (fscanf(file, "%15s %d", section, &edge_count) != 2 ||
        strcmp(section, "EDGES") != 0 || edge_count < 1 || edge_count > MSP_MAX_EDGES) {
        fclose(file);
        return MSP_ERROR_FORMAT;
    }
    for (i = 0; i < edge_count; ++i) {
        int from_id;
        int to_id;
        double weight;
        if (fscanf(file, "%d %d %lf", &from_id, &to_id, &weight) != 3 || weight <= 0.0 ||
            graph_add_edge(graph, from_id, to_id, weight, 1) != MSP_OK) {
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
            written = fprintf(file, "%d %s %.17g %.17g %s %.6g %.6g %.6g\n",
                              node->id, node->name, node->x, node->y,
                              storage_node_type_name(node->type), node->width,
                              node->depth, node->height);
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
        if (fprintf(file, "%d %d %.17g\n", edge->from_id, edge->to_id,
                    edge->weight) < 0) {
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
