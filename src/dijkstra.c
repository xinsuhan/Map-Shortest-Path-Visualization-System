#include "dijkstra.h"

#include "graph.h"

#include <stddef.h>

static void reset_result(PathResult *result) {
    int i;
    result->found = 0;
    result->total_distance = MSP_INFINITY;
    result->path_length = 0;
    result->visited_count = 0;
    for (i = 0; i < MSP_MAX_NODES; ++i) {
        result->path[i] = -1;
        result->visited_order[i] = -1;
    }
}

static void build_path(const Graph *graph, int goal_index, const int previous[], PathResult *result) {
    int reversed[MSP_MAX_NODES];
    int length = 0;
    int current = goal_index;
    int i;

    while (current >= 0 && length < MSP_MAX_NODES) {
        reversed[length++] = graph->nodes[current].id;
        current = previous[current];
    }
    for (i = 0; i < length; ++i) {
        result->path[i] = reversed[length - i - 1];
    }
    result->path_length = length;
}

int dijkstra_find_path(const Graph *graph, int start_id, int goal_id, PathResult *result) {
    double distance[MSP_MAX_NODES];
    int previous[MSP_MAX_NODES];
    int visited[MSP_MAX_NODES] = {0};
    int start_index;
    int goal_index;
    int i;

    if (graph == NULL || result == NULL) {
        return MSP_ERROR_INVALID_ARGUMENT;
    }
    reset_result(result);
    start_index = graph_find_node_index(graph, start_id);
    goal_index = graph_find_node_index(graph, goal_id);
    if (start_index < 0 || goal_index < 0) {
        return MSP_ERROR_NOT_FOUND;
    }

    for (i = 0; i < graph->node_count; ++i) {
        distance[i] = MSP_INFINITY;
        previous[i] = -1;
    }
    distance[start_index] = 0.0;

    for (;;) {
        int current = -1;
        double best = MSP_INFINITY;
        int neighbor;

        for (i = 0; i < graph->node_count; ++i) {
            if (!visited[i] && distance[i] < best) {
                best = distance[i];
                current = i;
            }
        }
        if (current < 0) {
            break;
        }

        visited[current] = 1;
        result->visited_order[result->visited_count++] = graph->nodes[current].id;
        if (current == goal_index) {
            result->found = 1;
            result->total_distance = distance[current];
            build_path(graph, current, previous, result);
            return MSP_OK;
        }

        for (neighbor = 0; neighbor < graph->node_count; ++neighbor) {
            double weight = graph_get_weight(graph, current, neighbor);
            double candidate = distance[current] + weight;
            if (!visited[neighbor] && weight < MSP_INFINITY && candidate < distance[neighbor]) {
                distance[neighbor] = candidate;
                previous[neighbor] = current;
            }
        }
    }
    return MSP_ERROR_NO_PATH;
}
