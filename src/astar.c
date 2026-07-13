#include "astar.h"

#include "graph.h"
#include "path_result.h"

#include <math.h>
#include <stddef.h>

static double euclidean_distance(const Node *from, const Node *goal) {
    double dx = from->x - goal->x;
    double dy = from->y - goal->y;
    return sqrt(dx * dx + dy * dy);
}

static double heuristic_scale(const Graph *graph) {
    double scale = 1.0;
    int i;

    for (i = 0; i < graph->edge_count; ++i) {
        if (!graph->edges[i].walkable) {
            continue;
        }
        const Node *from = graph_get_node(graph, graph->edges[i].from_id);
        const Node *to = graph_get_node(graph, graph->edges[i].to_id);
        double geometric_distance;
        double ratio;

        if (from == NULL || to == NULL) {
            continue;
        }
        geometric_distance = euclidean_distance(from, to);
        if (geometric_distance <= MSP_EPSILON) {
            continue;
        }
        ratio = graph->edges[i].weight / geometric_distance;
        if (ratio < scale) {
            scale = ratio;
        }
    }
    return scale < 0.0 ? 0.0 : scale;
}

static double heuristic(const Node *from, const Node *goal, double scale) {
    return euclidean_distance(from, goal) * scale;
}

int astar_find_path(const Graph *graph, int start_id, int goal_id, PathResult *result) {
    double g_score[MSP_MAX_NODES];
    double f_score[MSP_MAX_NODES];
    int previous[MSP_MAX_NODES];
    int closed[MSP_MAX_NODES] = {0};
    int start_index;
    int goal_index;
    double scale;
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
    scale = heuristic_scale(graph);

    for (i = 0; i < graph->node_count; ++i) {
        g_score[i] = MSP_INFINITY;
        f_score[i] = MSP_INFINITY;
        previous[i] = -1;
    }
    g_score[start_index] = 0.0;
    f_score[start_index] = heuristic(&graph->nodes[start_index], &graph->nodes[goal_index], scale);

    for (;;) {
        int current = -1;
        double best = MSP_INFINITY;
        int neighbor;

        for (i = 0; i < graph->node_count; ++i) {
            if (!closed[i] && f_score[i] < best) {
                best = f_score[i];
                current = i;
            }
        }
        if (current < 0) {
            break;
        }

        closed[current] = 1;
        if (result->visited_count >= MSP_MAX_NODES) return MSP_ERROR_CAPACITY;
        result->visited_order[result->visited_count++] = graph->nodes[current].id;
        if (current == goal_index) {
            result->found = 1;
            result->total_distance = g_score[current];
            return path_result_build(graph, current, previous, result);
        }

        for (neighbor = 0; neighbor < graph->node_count; ++neighbor) {
            double weight = graph_get_weight(graph, current, neighbor);
            double candidate = g_score[current] + weight;
            if (closed[neighbor] || weight >= MSP_INFINITY) {
                continue;
            }
            if (candidate < g_score[neighbor]) {
                previous[neighbor] = current;
                g_score[neighbor] = candidate;
                f_score[neighbor] = candidate +
                                    heuristic(&graph->nodes[neighbor], &graph->nodes[goal_index], scale);
            }
        }
    }
    return MSP_ERROR_NO_PATH;
}
