#include "dijkstra.h"

#include "graph.h"
#include "path_result.h"

#include <stddef.h>

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
    path_result_reset(result);
    if (graph->node_count < 0 || graph->node_count > MSP_MAX_NODES) {
        return MSP_ERROR_CAPACITY;
    }
    start_index = graph_find_node_index(graph, start_id);
    goal_index = graph_find_node_index(graph, goal_id);
    if (start_index < 0 || goal_index < 0) {
        return MSP_ERROR_NOT_FOUND;
    }
    if (start_index == goal_index) {
        return MSP_ERROR_INVALID_ARGUMENT;
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
        if (result->visited_count >= MSP_MAX_NODES) return MSP_ERROR_CAPACITY;
        result->visited_order[result->visited_count++] = graph->nodes[current].id;
        if (current == goal_index) {
            result->found = 1;
            result->total_distance = distance[current];
            return path_result_build(graph, current, previous, result);
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
