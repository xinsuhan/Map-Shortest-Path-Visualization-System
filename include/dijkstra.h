#ifndef MAP_SHORTEST_PATH_DIJKSTRA_H
#define MAP_SHORTEST_PATH_DIJKSTRA_H

#include "models.h"

int dijkstra_find_path(const Graph *graph, int start_id, int goal_id, PathResult *result);

#endif

