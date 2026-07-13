/* AI-assisted modification: added dynamic PathResult allocation support. */
#include "path_result.h"

#include <stdlib.h>
#include <string.h>

PathResult *path_result_create(void) {
    PathResult *result = (PathResult *)malloc(sizeof(*result));
    if (result == NULL) {
        return NULL;
    }
    memset(result, 0, sizeof(*result));
    return result;
}

void path_result_destroy(PathResult *result) {
    free(result);
}

void path_result_reset(PathResult *result) {
    int i;
    if (result == NULL) {
        return;
    }
    result->found = 0;
    result->total_distance = MSP_INFINITY;
    result->path_length = 0;
    result->visited_count = 0;
    for (i = 0; i < MSP_MAX_NODES; ++i) {
        result->path[i] = -1;
        result->visited_order[i] = -1;
    }
}

int path_result_build(const Graph *graph, int goal_index,
                      const int previous[], PathResult *result) {
    int reversed[MSP_MAX_NODES];
    int length = 0;
    int current = goal_index;
    int i;

    if (graph == NULL || previous == NULL || result == NULL ||
        goal_index < 0 || goal_index >= graph->node_count ||
        graph->node_count < 0 || graph->node_count > MSP_MAX_NODES) {
        return MSP_ERROR_INVALID_ARGUMENT;
    }
    while (current >= 0) {
        if (current >= graph->node_count || length >= MSP_MAX_NODES) {
            result->path_length = 0;
            return MSP_ERROR_FORMAT;
        }
        reversed[length++] = graph->nodes[current].id;
        current = previous[current];
    }
    for (i = 0; i < length; ++i) {
        result->path[i] = reversed[length - i - 1];
    }
    result->path_length = length;
    return MSP_OK;
}
