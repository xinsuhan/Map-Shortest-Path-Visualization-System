#ifndef MAP_SHORTEST_PATH_ASTAR_H
#define MAP_SHORTEST_PATH_ASTAR_H

#include "models.h"

int astar_find_path(const Graph *graph, int start_id, int goal_id, PathResult *result);

#endif

