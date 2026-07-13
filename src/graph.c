/* AI-assisted modification: added duplicate-edge validation. */
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

int graph_add_road_edge(Graph *graph, int from_id, int to_id, double weight,
                        RoadType type, int walkable, int bidirectional) {
    int from_index;
    int to_index;

    if (graph == NULL || from_id == to_id || weight <= 0.0 ||
        type < ROAD_MAIN || type > ROAD_BRIDGE ||
        (walkable != 0 && walkable != 1) ||
        (bidirectional != 0 && bidirectional != 1)) {
        return MSP_ERROR_INVALID_ARGUMENT;
    }
    from_index = graph_find_node_index(graph, from_id);
    to_index = graph_find_node_index(graph, to_id);
    if (from_index < 0 || to_index < 0) {
        return MSP_ERROR_NOT_FOUND;
    }
    {
        int i;
        for (i = 0; i < graph->edge_count; ++i) {
            const Edge *edge = &graph->edges[i];
            if ((edge->from_id == from_id && edge->to_id == to_id) ||
                (bidirectional && edge->from_id == to_id && edge->to_id == from_id)) {
                return MSP_ERROR_DUPLICATE;
            }
        }
    }
    if (graph->edge_count >= MSP_MAX_EDGES) {
        return MSP_ERROR_CAPACITY;
    }

    if (walkable) {
        graph->adjacency[from_index][to_index] = weight;
        if (bidirectional) {
            graph->adjacency[to_index][from_index] = weight;
        }
    }

    graph->edges[graph->edge_count].from_id = from_id;
    graph->edges[graph->edge_count].to_id = to_id;
    graph->edges[graph->edge_count].weight = weight;
    graph->edges[graph->edge_count].type = type;
    graph->edges[graph->edge_count].walkable = walkable ? 1 : 0;
    graph->edges[graph->edge_count].geometry_start = -1;
    graph->edges[graph->edge_count].geometry_count = 0;
    graph->edge_count++;
    return MSP_OK;
}

int graph_append_edge_geometry_point(Graph *graph, int edge_index, double x, double y) {
    Edge *edge;
    if (graph == NULL || edge_index < 0 || edge_index >= graph->edge_count) {
        return MSP_ERROR_INVALID_ARGUMENT;
    }
    if (graph->geometry_point_count >= MSP_MAX_GEOMETRY_POINTS) {
        return MSP_ERROR_CAPACITY;
    }
    edge = &graph->edges[edge_index];
    if (edge->geometry_count == 0) {
        edge->geometry_start = graph->geometry_point_count;
    } else if (edge->geometry_start + edge->geometry_count !=
               graph->geometry_point_count) {
        return MSP_ERROR_INVALID_ARGUMENT;
    }
    graph->geometry_points[graph->geometry_point_count].x = x;
    graph->geometry_points[graph->geometry_point_count].y = y;
    graph->geometry_point_count++;
    edge->geometry_count++;
    return MSP_OK;
}

int graph_add_edge(Graph *graph, int from_id, int to_id, double weight, int bidirectional) {
    return graph_add_road_edge(graph, from_id, to_id, weight, ROAD_MAIN, 1, bidirectional);
}

double graph_get_weight(const Graph *graph, int from_index, int to_index) {
    if (graph == NULL || from_index < 0 || to_index < 0 ||
        from_index >= graph->node_count || to_index >= graph->node_count) {
        return MSP_INFINITY;
    }
    return graph->adjacency[from_index][to_index];
}

