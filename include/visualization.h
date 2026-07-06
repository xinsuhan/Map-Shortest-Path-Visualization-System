#ifndef MAP_SHORTEST_PATH_VISUALIZATION_H
#define MAP_SHORTEST_PATH_VISUALIZATION_H

#include "models.h"

void visualization_print_graph(const Graph *graph);
void visualization_print_search(const Graph *graph, const PathResult *result);
void visualization_print_path(const Graph *graph, const PathResult *result);

#endif

