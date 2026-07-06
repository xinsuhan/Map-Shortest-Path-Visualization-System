#include "graph.h"

#include <string.h>

void graph_init(Graph *graph) {
    int i;
    int j;

    if (graph == NULL) {
        return;
    }

    memset(graph, 0, sizeof(*graph));
    for (i = 0; i < MSP_MAX_NODES; ++i) {
        for (j = 0; j < MSP_MAX_NODES; ++j) {
            graph->adjacency[i][j] = (i == j) ? 0.0 : MSP_INFINITY;
        }
    }
}

int graph_find_node_index(const Graph *graph, int node_id) {
    int i;

    if (graph == NULL) {
        return -1;
    }
    for (i = 0; i < graph->node_count; ++i) {
        if (graph->nodes[i].id == node_id) {
            return i;
        }
    }
    return -1;
}

const Node *graph_get_node(const Graph *graph, int node_id) {
    int index = graph_find_node_index(graph, node_id);
    return index >= 0 ? &graph->nodes[index] : NULL;
}

int graph_add_node(Graph *graph, Node node) {
    if (graph == NULL || node.name[0] == '\0') {
        return MSP_ERROR_INVALID_ARGUMENT;
    }
    if (graph->node_count >= MSP_MAX_NODES) {
        return MSP_ERROR_CAPACITY;
    }
    if (graph_find_node_index(graph, node.id) >= 0) {
        return MSP_ERROR_DUPLICATE;
    }

    graph->nodes[graph->node_count] = node;
    graph->node_count++;
    return MSP_OK;
}

int graph_add_edge(Graph *graph, int from_id, int to_id, double weight, int bidirectional) {
    int from_index;
    int to_index;

    if (graph == NULL || weight < 0.0) {
        return MSP_ERROR_INVALID_ARGUMENT;
    }
    if (graph->edge_count >= MSP_MAX_EDGES) {
        return MSP_ERROR_CAPACITY;
    }

    from_index = graph_find_node_index(graph, from_id);
    to_index = graph_find_node_index(graph, to_id);
    if (from_index < 0 || to_index < 0) {
        return MSP_ERROR_NOT_FOUND;
    }

    graph->adjacency[from_index][to_index] = weight;
    if (bidirectional) {
        graph->adjacency[to_index][from_index] = weight;
    }

    graph->edges[graph->edge_count].from_id = from_id;
    graph->edges[graph->edge_count].to_id = to_id;
    graph->edges[graph->edge_count].weight = weight;
    graph->edge_count++;
    return MSP_OK;
}

double graph_get_weight(const Graph *graph, int from_index, int to_index) {
    if (graph == NULL || from_index < 0 || to_index < 0 ||
        from_index >= graph->node_count || to_index >= graph->node_count) {
        return MSP_INFINITY;
    }
    return graph->adjacency[from_index][to_index];
}

