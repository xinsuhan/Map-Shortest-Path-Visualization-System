#include "visualization.h"

#include "graph.h"

#include <stdio.h>

void visualization_print_graph(const Graph *graph) {
    int i;
    if (graph == NULL) {
        return;
    }
    printf("\nNodes (%d):\n", graph->node_count);
    for (i = 0; i < graph->node_count; ++i) {
        printf("  [%d] %-16s (%.1f, %.1f)\n", graph->nodes[i].id,
               graph->nodes[i].name, graph->nodes[i].x, graph->nodes[i].y);
    }
    printf("Edges (%d):\n", graph->edge_count);
    for (i = 0; i < graph->edge_count; ++i) {
        printf("  %d <-> %d  distance=%.2f\n", graph->edges[i].from_id,
               graph->edges[i].to_id, graph->edges[i].weight);
    }
}

void visualization_print_search(const Graph *graph, const PathResult *result) {
    int i;
    if (graph == NULL || result == NULL) {
        return;
    }
    printf("Visited: ");
    for (i = 0; i < result->visited_count; ++i) {
        const Node *node = graph_get_node(graph, result->visited_order[i]);
        printf("%s%s", node != NULL ? node->name : "?",
               i + 1 < result->visited_count ? " -> " : "\n");
    }
}

void visualization_print_path(const Graph *graph, const PathResult *result) {
    int i;
    if (graph == NULL || result == NULL || !result->found) {
        printf("No path found.\n");
        return;
    }
    printf("Shortest path: ");
    for (i = 0; i < result->path_length; ++i) {
        const Node *node = graph_get_node(graph, result->path[i]);
        printf("%s%s", node != NULL ? node->name : "?",
               i + 1 < result->path_length ? " -> " : "\n");
    }
    printf("Total distance: %.2f\n", result->total_distance);
}

