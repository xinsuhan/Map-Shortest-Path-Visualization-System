/* AI-assisted modification: added map persistence support. */
#include "storage.h"

#include "graph.h"

#include <stdio.h>
#include <string.h>

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
        int parsed;
        line_number++;
        trim_newline(line);
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        parsed = sscanf(line, "%d,%63[^,],%lf,%lf", &node.id, node.name, &node.x, &node.y);
        if (parsed != 4) {
            if (line_number == 1) {
                continue;
            }
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
    for (i = 0; i < node_count; ++i) {
        Node node;
        if (fscanf(file, "%d %63s %lf %lf", &node.id, node.name, &node.x, &node.y) != 4 ||
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
        if (fprintf(file, "%d %s %.17g %.17g\n", node->id, node->name,
                    node->x, node->y) < 0) {
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
